"""Build the static table exporter using only frozen policy dependencies."""
import json
import subprocess
from pathlib import Path

OUT = Path(__file__).resolve().parent
TREE = OUT / 'source_tree'
EXP = TREE / 'experiments/v6/sep07_compositions_v0'
command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(TREE),
           str(OUT / 'export_policy.cpp'), str(EXP / 'runs/animal_group_policy_sep08_001/source/library.cpp'),
           str(EXP / 'runs/animal_repair_sep08_001/source/repair.cpp'), '-o', str(OUT / 'export_policy')]
with (OUT / 'export_build.log').open('w') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
with (OUT / 'exported_data.json').open('w') as data:
    subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(OUT / 'export_policy')], stdout=data, check=True)
report = json.loads((OUT / 'exported_data.json').read_text())
print({key: len(value) for key, value in report.items() if isinstance(value, list)})
