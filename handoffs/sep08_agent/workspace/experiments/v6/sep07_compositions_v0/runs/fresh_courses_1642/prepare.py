"""Import novel recent recorded courses and retain every donor attribution."""
import csv
import hashlib
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from generate_public_routes import triple
from verify_public_router import pack


def main():
    research = EXP / 'research/refresh_1642'
    assert (research / 'REFRESH.json').exists()
    target = RUN / 'library'
    assert not target.exists()
    (target / 'source').mkdir(parents=True)
    old = json.loads((EXP / 'league/top_replay_library/IMPORT.json').read_text())['programs']
    known = {(int(row['episode']), int(row['seat'])) for row in old}
    manifest = list(csv.DictReader((research / 'top_replay_manifest.csv').open()))
    episodes = defaultdict(list)
    for row in manifest:
        episodes[int(row['episode_id'])].append(row)
    programs, offsets, values, phenotype = [], [], [], {}
    with (RUN / 'source_cases.txt').open('w') as fixture:
        for episode, donors in sorted(episodes.items()):
            path = EXP / f'replays/episode-{episode}-replay.json'
            raw = path.read_bytes()
            replay = json.loads(raw)
            for donor in donors:
                seat = replay['info']['TeamNames'].index(donor['team'])
                if (episode, seat) in known:
                    continue
                actions = [step[seat]['action'] for step in replay['steps'][1:]]
                assert len(actions) == 719
                data, starts = [], []
                for action in actions:
                    starts.append(len(data))
                    units = [action['farmer'], *action['hands']]
                    data.extend([len(units), len(action['market'])])
                    for unit in units:
                        data.extend(triple(unit))
                    for order in action['market']:
                        data.extend(triple(order, True))
                normalized = hashlib.sha256(json.dumps(data, separators=(',', ':')).encode()).hexdigest()
                origin = {'episode': episode, 'seat': seat, **donor,
                          'replay_sha256': hashlib.sha256(raw).hexdigest(),
                          'source_cash': replay['rewards'][seat],
                          'source_rival_cash': replay['rewards'][seat ^ 1]}
                if normalized in phenotype:
                    programs[phenotype[normalized]]['donors'].append(origin)
                    continue
                program = len(programs)
                phenotype[normalized] = program
                offsets.append([start + len(values) for start in starts])
                values.extend(data)
                programs.append({'program': program, 'normalized_action_sha256': normalized, 'donors': [origin]})
                for step, action in enumerate(actions):
                    observation = replay['steps'][step][seat]['observation']
                    observation['step'] = step
                    line = pack(observation, action, step == 0)
                    if step == 0:
                        line = str(program + 1) + ' ' + line.split(' ', 1)[1]
                    fixture.write(line)
    assert programs
    code = '// Exact public recorded courses, with full attribution in IMPORT.json.\n'
    code += f'inline constexpr int offsets[{len(programs)}][719]={{\n'
    code += ',\n'.join('{' + ','.join(map(str, row)) + '}' for row in offsets) + '\n};\n'
    code += 'inline constexpr int values[]={\n'
    code += ',\n'.join(','.join(map(str, values[i:i + 100])) for i in range(0, len(values), 100)) + '\n};\n'
    (target / 'source/tapes.inc').write_text(code)
    origins = {}
    for name in ('agent.hpp', 'agent.cpp'):
        source = EXP / 'league/top_replay_library/source' / name
        origins[str(source.relative_to(EXP))] = hashlib.sha256(source.read_bytes()).hexdigest()
        (target / 'source' / name).write_text(source.read_text().replace('top_replay_library', 'fresh_courses_1642'))
    source = EXP / 'tests/public_router_parity.cpp'
    verifier = '\n'.join(line for line in source.read_text().splitlines() if not line.startswith('#include "../league/'))
    verifier = '#include "library/source/agent.hpp"\n' + verifier.replace('top_replay_library', 'fresh_courses_1642') + '\n'
    (RUN / 'parity.cpp').write_text(verifier)
    for path in (source, EXP / 'scripts/generate_public_routes.py', EXP / 'scripts/verify_public_router.py'):
        origins[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
    record = {'programs': programs, 'conversion_source_sha256': origins,
              'scope': 'Exact single observed courses; no private branching code inferred.',
              'changes': ['C++ numeric conversion with engine-ignored order arguments normalized',
                          'Worker action count follows current observation', 'Distinct namespace; existing library untouched'],
              'reuse': 'User explicitly authorized borrowing complete public replay schedules; no separate code license supplied.',
              'parity_status': 'Fixture generated; C++ parity must pass before the screen.'}
    (target / 'IMPORT.json').write_text(json.dumps(record, indent=2) + '\n')
    (target / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'fresh_courses_1642',
        'header': 'source/agent.hpp', 'type': 'compositions::fresh_courses_1642::Agent',
        'sources': ['source/agent.cpp']}, indent=2) + '\n')
    (target / 'README.md').write_text('# Recent top-player course library\n\nDistinct recorded courses, selected by constructor index. IMPORT.json preserves every donor and exact conversion lineage. These are complete observed schedules, not reconstructed private branching agents. Source parity and transfer checks are stored in the parent run.\n')
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-DVERIFY_LIBRARY',
               '-I', str(ROOT), str(RUN / 'parity.cpp'), str(target / 'source/agent.cpp'), '-o', str(RUN / 'parity')]
    (RUN / 'PARITY_COMMAND.json').write_text(json.dumps(command, indent=2) + '\n')
    subprocess.run(command, check=True)
    command = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'parity'), str(RUN / 'source_cases.txt')]
    result = subprocess.run(command, check=True, text=True, capture_output=True)
    (RUN / 'PARITY.json').write_text(json.dumps({'programs': len(programs), 'actions': 719 * len(programs),
        'result': result.stdout.strip(), 'command': command}, indent=2) + '\n')
    print('Imported', len(programs), 'distinct courses;', result.stdout.strip())


if __name__ == '__main__':
    main()
