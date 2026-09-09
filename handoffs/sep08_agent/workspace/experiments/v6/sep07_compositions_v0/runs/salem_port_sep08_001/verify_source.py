"""Compare the reviewed public controller with its C++ translation."""
from collections import Counter
from copy import deepcopy
from datetime import datetime, timezone
from pathlib import Path
import ast
import hashlib
import json
import subprocess
import sys

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from verify_public_router import pack

lineage = json.loads((RUN / 'LINEAGE.json').read_text())
reference = EXP / lineage['source_file']
tree = ast.parse(reference.read_text())
entry = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'agent')
assert len(entry.body) == 1 and isinstance(entry.body[0], ast.Try)
entry.body = entry.body[0].body
namespace = {}
exec(compile(tree, str(reference), 'exec'), namespace)
counts = Counter()
paths = sorted((EXP / 'replays').glob('*replay.json'))[:4]
assert len(paths) == 4
replays = [json.loads(p.read_text()) for p in paths]
fixture = RUN / 'source_cases.txt'
with fixture.open('x') as out:
    for forced in (False, True):
        for replay in replays:
            for seat in range(2):
                for step, pair in enumerate(replay['steps'][:719]):
                    obs = deepcopy(pair[seat]['observation'])
                    obs['step'] = step
                    assert obs['player'] == seat
                    if forced:
                        own = obs['farms'][seat]
                        for x, y in [own['farmer'], *own['hands']]:
                            own['tiles'][y][x] = {'kind': 'WEED'}
                        obs['private']['shed'] = {p: 20 for p in namespace['_FR_ITEMS']}
                        obs['town']['unlocked_shops'] = ['YARN_STORE'] if step % 48 < 24 else ['BAKERY']
                    pending = namespace['_FR_STATE'][seat]
                    counts['repay_turns'] += pending['due_step'] == step and bool(pending['due'])
                    action = namespace['agent'](obs)
                    repairs = namespace['_WEED_STATE'][seat]['active']
                    counts['weed_starts'] += sum(v['start'] == step for v in repairs.values())
                    counts['weed_replay_actions'] += sum(2 <= step-v['start'] <= 9 for v in repairs.values())
                    counts['advance_turns'] += namespace['_FR_STATE'][seat]['due_step'] == step + 1
                    counts['forced_cases' if forced else 'recorded_cases'] += 1
                    out.write(pack(obs, action, step == 0, full_tiles=True))
assert all(counts[k] > 0 for k in ['repay_turns', 'weed_starts', 'weed_replay_actions', 'advance_turns'])
source = (EXP / 'tests/public_router_parity.cpp').read_text()
source = '#define VERIFY_KING 1\n#include "proposals/salem_sep08_m3/source/agent.hpp"\n' + source[source.index('#include <fstream>'):]
source = source.replace('compositions::king_rc4::Agent agent;', 'compositions::salem_sep08_m3::Agent agent;')
(RUN / 'parity.cpp').write_text(source)
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(ROOT),
     str(RUN / 'parity.cpp'), str(RUN / 'source/policy.cpp'), '-o', str(RUN / 'parity')],
    ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'parity'), str(fixture)],
]
for i, command in enumerate(commands):
    result = subprocess.run(command, capture_output=True, text=True)
    (RUN / f'parity_command{i}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'counts': dict(counts),
    'result': result.stdout.strip(), 'commands': commands,
    'reference_sha256': hashlib.sha256(reference.read_bytes()).hexdigest(),
    'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
    'replays': [str(p.relative_to(EXP)) for p in paths],
    'scope': 'Eight full recorded and eight forced observation sequences. Force weed repairs, stock, demand and sale repayment. Remove only the public outer exception fallback to fail on reference errors. Source parity does not establish strength.'}
(RUN / 'SOURCE_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
