from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
package = EXP / 'runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2'
manifest = json.loads((package / 'agent.json').read_text())
sources = sorted({str((package / source).resolve()) for source in manifest['sources']})
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O3', '-DNDEBUG', '-march=native',
     '-mtune=native', '-fno-exceptions', '-fno-rtti', '-fno-math-errno', '-fno-semantic-interposition',
     '-fno-plt', '-flto', '-pthread', '-I', str(ROOT), str(RUN / 'trace.cpp'), *sources, '-o', str(RUN / 'trace')],
    ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'trace'), str(RUN / 'trace_results')],
]
(RUN / 'TRACE_COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'trace_command{i}.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
print('Two full native traces and one-step order counterfactuals complete.')
