"""One-time packaging: copy a sealed result, preserving all core binary bytes."""
import hashlib
import json
from pathlib import Path
import shutil
import sys

PACKAGE = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    seal = Path(sys.argv[1]).resolve()
    assert not (PACKAGE / 'manifest.json').exists(), 'Do not overwrite a completed release'
    if (PACKAGE / 'runtime/fast_solver_cli').exists():
        assert digest(PACKAGE / 'runtime/fast_solver_cli') == digest(seal / 'runtime/fast_solver_cli')
    for line in (seal / 'SHA256SUMS').read_text().splitlines():
        sha, path = line.split('  ', 1)
        assert digest(seal / path) == sha, path
    origins = {}

    def copy(src, dest):
        target = PACKAGE / dest
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, target)
        target.chmod(target.stat().st_mode | 0o200)
        origins[str(dest)] = {'source': str(src), 'sha256': digest(src)}

    for folder in ('runtime', 'lib', 'evidence', 'tests/fixtures'):
        for src in (seal / folder).rglob('*'):
            if src.is_file():
                copy(src, src.relative_to(seal))
    for name in ('solve.sh', 'solve_reference.sh', 'objective.md'):
        copy(seal / name, Path(name))
    for name in ('manifest.json', 'provenance.json', 'SHA256SUMS'):
        copy(seal / name, Path('evidence/original_seal') / name)
    source = seal / 'source/repository/experiments/v6/sep2_day_solver/work'
    # Keep project C++ and dependency headers. Offline experiment scripts and
    # their original CMake entry points are not this package's build system.
    for src in source.rglob('*'):
        if src.is_file() and src.suffix in ('.cpp', '.hpp', '.h', '.ipp', '.tpp', '.inc'):
            if '/native_cpp_sdk/ortools/' not in str(src):
                copy(src, Path('src/components') / src.relative_to(source))
    for src in (seal / 'source/repository/fast_game_engine').rglob('*'):
        if src.is_file():
            copy(src, Path('include/fast_game_engine') / src.relative_to(seal / 'source/repository/fast_game_engine'))
    for src in (seal / 'source/sdk/include').rglob('*'):
        if src.is_file():
            copy(src, Path('vendor/include') / src.relative_to(seal / 'source/sdk/include'))
    for src in (seal / 'source/build_metadata').rglob('*'):
        if src.is_file():
            copy(src, Path('evidence/build_metadata') / src.name)
    cmake = (seal / 'CMakeLists.txt').read_text()
    links = cmake.split('target_link_libraries(day_scheduler PUBLIC\n', 1)[1].split('\n)', 1)[0]
    (PACKAGE / 'cmake').mkdir(exist_ok=True)
    (PACKAGE / 'cmake/bundled_libraries.cmake').write_text(
        'set(DAY_SOLVER_BUNDLED_LIBRARIES\n' + links.replace('${CMAKE_CURRENT_SOURCE_DIR}', '${DAY_SOLVER_DIR}') + '\n)\n')
    header = (seal / 'api/scheduler.hpp').read_text()
    header = header.replace('../source/repository/experiments/v6/sep2_day_solver/work/resource_portfolio_order_v2/solver/include/day_solver_api.hpp', 'day_solver_api.hpp')
    (PACKAGE / 'include/day_solver').mkdir(parents=True, exist_ok=True)
    (PACKAGE / 'include/day_solver/scheduler.hpp').write_text(header)
    cpp = (seal / 'api/scheduler.cpp').read_text().replace('"scheduler.hpp"', '"day_solver/scheduler.hpp"')
    cpp = cpp.replace('../source/repository/experiments/v6/sep2_day_solver/work/native_quick_portfolio_v30/quick.hpp', 'components/native_quick_portfolio_v30/quick.hpp')
    (PACKAGE / 'src/scheduler.cpp').write_text(cpp)
    test = (seal / 'api/test_scheduler.cpp').read_text().replace('"scheduler.hpp"', '"day_solver/scheduler.hpp"')
    test = test.replace('../source/repository/experiments/v6/sep2_day_solver/work/resource_portfolio_order_v2/solver/include/', '')
    (PACKAGE / 'tests/test_scheduler.cpp').write_text(test)

    # Include the public benchmark inputs so the recorded V30 groups can be
    # replayed without the experiment directory or original schedules.
    cases, groups = {}, {}
    for group, folder in [('quick', 'full_v1'), ('broader', 'broader/results_v1'), ('slow', 'tail242_v1')]:
        plan = json.loads((seal / 'evidence/native_quick_portfolio_v30' / folder / 'plan.json').read_text())
        selected = plan['cases'] if isinstance(plan['cases'], dict) else {
            row['case_id']: row['problem'] for row in plan['cases']}
        groups[group] = list(selected)
        for name, path in selected.items():
            src = Path(path)
            if name in cases:
                assert cases[name]['sha256'] == digest(src)
                continue
            dest = Path('benchmarks/cases') / (name + '.json')
            copy(src, dest)
            cases[name] = dict(path=str(dest), sha256=digest(src))
    combined = list(dict.fromkeys(groups['quick'] + groups['broader']))
    groups['development'] = [n for n in combined if n[0].isdigit()]
    groups['synthetic'] = [n for n in combined if not n[0].isdigit()]
    groups['smoke'] = [p.stem for p in sorted((PACKAGE / 'tests/fixtures').glob('*.json'))]
    assert len(groups['development']) == 323 and len(groups['synthetic']) == 9 and len(groups['slow']) == 242
    for p in (PACKAGE / 'tests/fixtures').glob('*.json'):
        if p.stem not in cases:
            cases[p.stem] = dict(path=str(p.relative_to(PACKAGE)), sha256=digest(p))
    (PACKAGE / 'benchmarks/cases.json').write_text(json.dumps(dict(groups=groups, cases=cases), indent=2) + '\n')
    origins_path = PACKAGE / 'evidence/import_origins.json'
    origins_path.write_text(json.dumps(origins, indent=2) + '\n')
    print('Imported', len(origins), 'unchanged files and', len(cases), 'public benchmark inputs.')


if __name__ == '__main__':
    main()
