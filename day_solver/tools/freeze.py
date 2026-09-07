"""Write package metadata and checksums after packaging/verification edits."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files():
    for path in sorted(PACKAGE.rglob('*')):
        if not path.is_file():
            continue
        rel = path.relative_to(PACKAGE)
        if rel.parts[0].startswith('build') or rel.parts[0] == 'work' or '__pycache__' in rel.parts:
            continue
        if path.suffix in ('.log', '.pyc') or rel == Path('SHA256SUMS'):
            continue
        yield path


def main():
    original = json.loads((PACKAGE / 'evidence/original_seal/manifest.json').read_text())
    main_binary = digest(PACKAGE / 'runtime/fast_solver_cli')
    assert main_binary == original['selected_binary_sha256']
    assert digest(PACKAGE / 'runtime/reference_solver_cli') == original['reference_binary_sha256']
    assert digest(PACKAGE / 'objective.md') == original['objective_sha256']
    manifest = dict(format_version=1, name='kaggriculture-day-solver', package_version='1.0.0',
        created_at_utc=datetime.now(timezone.utc).isoformat(),
        backend='native_quick_portfolio_v30', reference_backend='native_quick_portfolio_v20',
        input_schema='schemas/day_problem_v3.schema.json', contract='objective.md',
        cli='solve.sh', cpp_header='include/day_solver/scheduler.hpp', cmake_target='DaySolver::scheduler',
        trained_model=False, original_route_input=False, options=original['options'],
        source_binary_sha256=main_binary, reference_binary_sha256=original['reference_binary_sha256'],
        objective_sha256=original['objective_sha256'],
        engine_headers={n:digest(PACKAGE/'include/fast_game_engine'/n) for n in ('sim.hpp','pyrandom.hpp')},
        platform=dict(os='Linux', architecture='x86_64', validated_distribution='Ubuntu 24.04',
                      compiler='GCC 13.3', benchmark_cpu='Intel Core i7-14700K',
                      system_dependencies='Compatible glibc and ELF loader', gpu_required=False),
        known_limits=dict(max_workers=40, board=[10,10], shed_capacity_ignored=True,
            random_night_weeds_excluded=True, budget_is_soft=True,
            economic_feasibility_external=True, v30_new_unseen_validation=False,
            unresolved_replay_days=['106126272_p0_d16','106139776_p1_d16']),
        benchmarks='benchmarks/cases.json', verification='evidence/verification/package_checks.json',
        frozen_files='SHA256SUMS', excluded=['/build*/','/work/','**/__pycache__/','*.log','*.pyc','/SHA256SUMS'],
        packaging_changes=['Relocatable layout, build and runtime wrappers',
            'Typed v3 preparation helper and consumer example',
            'Documentation, schema and exposed benchmark inputs',
            'Standard-library inventory task extractor, checked against saved ledgers'],
        solver_algorithm_changes=False)
    (PACKAGE / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    paths = list(files())
    (PACKAGE / 'SHA256SUMS').write_text(''.join(digest(p) + '  ' + str(p.relative_to(PACKAGE)) + '\n' for p in paths))
    print('Recorded', len(paths), 'package files; unchanged V30 executable', main_binary)


if __name__ == '__main__':
    main()
