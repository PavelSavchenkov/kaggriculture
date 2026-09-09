"""Derive a reviewable warm-compiler variant with necessary-condition skips."""
import difflib
import hashlib
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    source = EXP / "baselines/warm/runs/expansion_portfolio_sep08_001/next_compile/compile_early_inputs.cpp"
    target = EXP / "source/warm_compile_screened.cpp"
    output = EXP / "runs/warm_screen_integration_v1"
    output.mkdir(exist_ok=False)
    before = source.read_text(); after = before

    def replace(old, new):
        nonlocal after
        assert after.count(old) == 1
        after = after.replace(old, new)

    replace('#include "../../submission_losses_sep08_001/general_quadrant_compile/source/season.hpp"',
            '#include "baselines/warm/runs/submission_losses_sep08_001/general_quadrant_compile/source/season.hpp"\n#include "query_screen.hpp"\n#include <chrono>')
    replace('    for(int day=first;day<30;++day) {',
            '    std::ofstream screening(directory/"screening.csv");\n'
            '    screening<<"day,extra_hires,workers,reasons,capacity_bound,supply_bound,release_bound,screen_seconds\\n";\n'
            '    for(int day=first;day<30;++day) {')
    replace('            day_scheduler::Options budget;budget.seconds=seconds;budget.fallback_workers=1;',
            '            const auto screen_start=std::chrono::steady_clock::now();\n'
            '            const auto screen=labor::screen_query(trial,day==29?23:24);\n'
            '            const double screen_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-screen_start).count();\n'
            '            screening<<day<<\',\'<<hires<<\',\'<<trial.worker_count<<\',\'<<screen.reasons<<\',\'<<screen.capacity_bound<<\',\'<<screen.supply_bound<<\',\'<<screen.release_bound<<\',\'<<screen_seconds<<\'\\n\';screening.flush();\n'
            '            if(screen.reasons){\n'
            '                log<<day<<\',\'<<hires<<",0,0,0\\n";log.flush();\n'
            '                std::cout<<"day="<<day<<" hires="<<hires<<" necessary_condition_rejected="<<screen.reasons<<std::endl;\n'
            '                continue;\n'
            '            }\n'
            '            day_scheduler::Options budget;budget.seconds=seconds;budget.fallback_workers=1;')
    with target.open("x") as stream: stream.write(after)
    patch = "".join(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),
                                     fromfile=str(source.relative_to(EXP)), tofile=str(target.relative_to(EXP))))
    (output / "SOURCE.patch").write_text(patch)
    report = {"change": "After unchanged trial construction, save necessary-condition evidence and skip only rejected fixed-calendar workforces. Preserve candidate order, reuse, repair, cold solve, economic replay and stopping conditions.",
              "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "target_sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
              "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    (output / "SOURCE.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
