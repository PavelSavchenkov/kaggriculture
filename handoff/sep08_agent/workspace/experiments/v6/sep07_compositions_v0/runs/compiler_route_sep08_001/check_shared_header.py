"""Check that labor and route packages link together and keep their records."""
from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
names=['compiler_labor_mixed_m1','compiler_route_mixed_m0','compiler_route_p55_m1']
command=['conda','run','-n','kaggriculture','python',str(EXP/'scripts/build_arena.py'),'--agents',*names,'public_router']
build=subprocess.run(command,capture_output=True,text=True)
(RUN/'shared_header_build.log').write_text(build.stdout+build.stderr)
build.check_returncode()
binary=build.stdout.strip().splitlines()[-1]
output=RUN/'shared_header_checks';output.mkdir(exist_ok=False)
commands=[command]
count=0
for name in names:
    path=output/f'{name}.json'
    cmd=['conda','run','-n','kaggriculture',binary,'--a',name,'--b','public_router','--games','8',
         '--seed-start','1000','--seat-mode','both','--threads','4','--validate','--profile','--output',str(path)]
    subprocess.run(cmd,check=True);commands.append(cmd)
    old=(EXP/'runs/compiler_labor_sep08_001' if name.startswith('compiler_labor') else RUN)/f'discovery/{name}_vs_public_router.json'
    a=json.loads(path.read_text())['games'];b=json.loads(old.read_text())['games']
    assert a==b,name
    count+=len(a)
record=json.loads((RUN/'HEADER_AMENDMENT.json').read_text())
record['verification']={'mixed_package_generic_build':binary,'complete_records_exact':count,'commands':commands}
(RUN/'HEADER_AMENDMENT.json').write_text(json.dumps(record,indent=2)+'\n')
print('Canonical-header amendment verified:',count,'full records exact.')
