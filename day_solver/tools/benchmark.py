"""Repeat an exposed benchmark group; require strict replay and an hourly ledger."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import time

from inventory_ledger import compare_auditor, trace

PACKAGE = Path(__file__).resolve().parents[1]


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='new output directory')
    parser.add_argument('--group', default='smoke', choices=['smoke', 'quick', 'development', 'synthetic', 'slow'])
    parser.add_argument('--case', action='append', default=[], help='exposed catalog case; overrides group')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--seconds', type=float, default=900)
    parser.add_argument('--threads', type=int, default=8)
    parser.add_argument('--solver', type=Path, default=PACKAGE / 'build/day_solver_cli')
    parser.add_argument('--audit', type=Path, default=PACKAGE / 'build/day_solver_audit')
    args = parser.parse_args()
    if args.jobs < 1 or args.threads < 1 or not 0 <= args.seconds < float('inf'):
        parser.error('positive jobs/threads and a finite nonnegative budget are required')
    assert args.audit.is_file(), args.audit
    assert args.solver.is_file(), args.solver
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    catalog = json.loads((PACKAGE / 'benchmarks/cases.json').read_text())
    selected = {name: catalog['cases'][name] for name in (args.case or catalog['groups'][args.group])}
    for row in selected.values():
        assert hashlib.sha256((PACKAGE / row['path']).read_bytes()).hexdigest() == row['sha256']
    write(output / 'plan.json', dict(group=args.group, cases=selected, jobs=args.jobs,
        seconds=args.seconds, fallback_threads=args.threads, scope='Exposed regression; not unseen validation',
        solver_path=str(args.solver.resolve()),
        solver_sha256=hashlib.sha256(args.solver.read_bytes()).hexdigest(),
        auditor_sha256=hashlib.sha256(args.audit.read_bytes()).hexdigest()))

    def run(name, row):
        problem = PACKAGE / row['path']
        target = output / name
        started = time.perf_counter()
        completed = subprocess.run([str(PACKAGE / 'with_runtime.sh'), str(args.solver.resolve()), str(problem), str(target),
            str(args.seconds), str(args.threads)], capture_output=True, text=True,
            timeout=args.seconds + 60)
        wall = time.perf_counter() - started
        (output / (name + '.stderr.txt')).write_text(completed.stderr)
        report_file = target / 'report.json'
        report = json.loads(report_file.read_text()) if report_file.exists() else {'status': 'ERROR'}
        hours, strict = 0, False
        if completed.returncode == 0 and report['status'] == 'SCHEDULE':
            audit = subprocess.run([str(PACKAGE / 'with_runtime.sh'), str(args.audit.resolve()),
                str(problem), str(target / 'schedule.json'), '--json-trace'],
                capture_output=True, text=True, check=True)
            ledger = trace(json.loads(problem.read_text()), json.loads((target / 'schedule.json').read_text()))
            compare_auditor(ledger, json.loads(audit.stdout))
            hours = len(ledger['hours'])
            assert hours == 24
            strict = True
            (target / 'audit.json').write_text(audit.stdout)
            write(target / 'ledger.json', ledger)
        return dict(case=name, status=report['status'], returncode=completed.returncode,
            strict=strict, independent_hours=hours, solver_seconds=report.get('seconds'), wall_seconds=wall)

    rows, started = [], time.perf_counter()
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {executor.submit(run, name, row): name for name, row in selected.items()}
        for future in as_completed(futures):
            name = futures[future]
            try:
                row = future.result()
            except Exception as error:
                row = dict(case=name, strict=False, status='ERROR', error=str(error), independent_hours=0)
            rows.append(row)
            write(output / 'rows.json', sorted(rows, key=lambda r: r['case']))
            print(f"{len(rows)}/{len(selected)} {name}: {row['status']}", flush=True)
    values = [r['solver_seconds'] for r in rows if r.get('solver_seconds') is not None]
    summary = dict(scope='Exposed regression; not unseen validation', cases=len(rows),
        strict=sum(r['strict'] for r in rows), independent_hours=sum(r['independent_hours'] for r in rows),
        elapsed_seconds=time.perf_counter()-started,
        median_solver_seconds=statistics.median(values) if values else None,
        mean_solver_seconds=statistics.mean(values) if values else None,
        max_solver_seconds=max(values) if values else None,
        timing_scope='All completed reports, including UNKNOWN; excludes process startup and independent audit')
    write(output / 'summary.json', summary)
    print(json.dumps(summary), flush=True)
    raise SystemExit(0 if summary['strict'] == len(selected) else 1)


if __name__ == '__main__':
    main()
