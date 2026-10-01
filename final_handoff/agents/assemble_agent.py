"""Rebuild a runnable agent folder from final_handoff/agents/<name>: text sidecars + weights (sha256-checked) + bridge compiled from
source with the standalone CMake project.

    conda run -n kaggriculture --no-capture-output python final_handoff/agents/assemble_agent.py <name> <out dir> [--cxx <compiler>]

<name> is one of the folders next to this script. The Local-LB agents share their bridge source with a Kaggle agent (see TWIN).
The rebuilt bridge plays the same games as the shipped one. Its file hash differs, because build paths are embedded.
For the exact Kaggle bytes, use the .tar.gz archive in the agent folder. For a portable (old-glibc) bridge, pass the kagbuild
compiler, e.g. --cxx ~/anaconda3/envs/kagbuild/bin/x86_64-conda-linux-gnu-c++.
"""
import argparse
import hashlib
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
TWIN = {'locallb_f1_fc2ens': 'kaggle_56720831_honest1', 'locallb_m3_fc3nv': 'kaggle_56720080_base_m3'}
SKIP = {'checks', 'WEIGHTS.txt'}

parser = argparse.ArgumentParser()
parser.add_argument('name')
parser.add_argument('out', type=Path)
parser.add_argument('--cxx')
args = parser.parse_args()

src = HERE / args.name
out = args.out
if out.exists():
    raise SystemExit(f'{out} exists; choose a fresh folder')
out.mkdir(parents=True)
for p in src.iterdir():
    if p.name in SKIP or p.name.endswith('.tar.gz'):
        continue
    (shutil.copytree if p.is_dir() else shutil.copy2)(p, out / p.name)
twin = HERE / TWIN.get(args.name, args.name)
for part in ('source', 'standalone', 'main.py'):
    if not (out / part).exists():
        (shutil.copytree if (twin / part).is_dir() else shutil.copy2)(twin / part, out / part)

for line in (src / 'WEIGHTS.txt').read_text().splitlines():
    if not line.strip() or line.startswith('#'):
        continue
    target, _, rest = line.partition('  ->  ')
    weight, digest = rest.split()
    data = (ROOT / weight).read_bytes()
    if hashlib.sha256(data).hexdigest() != digest:
        raise SystemExit(f'sha256 mismatch for {weight}')
    (out / target).parent.mkdir(parents=True, exist_ok=True)
    (out / target).write_bytes(data)

build = out / '_build'
cmake = ['cmake', '-S', str(out / 'standalone'), '-B', str(build)] + ([f'-DCMAKE_CXX_COMPILER={args.cxx}'] if args.cxx else [])
subprocess.run(cmake, check=True)
subprocess.run(['cmake', '--build', str(build), '-j', '4'], check=True)
shutil.copy2(build / 'libopus_lb_bridge.so', out / 'libopus_lb_bridge.so')
print(f'assembled {args.name} -> {out} (run games with verify_submission-style loaders; main.py finds the bridge next to itself)')
