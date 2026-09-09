"""Freeze and evaluate complete animal policies with per-game diagnostics."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import csv
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
assert json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())['games']==72
fixtures=list(csv.DictReader(line for line in (RUN/'POLICY_FIXTURES.csv').read_text().splitlines() if line.strip()))
assert len(fixtures)==12 and all(row['pass']=='1' for row in fixtures)
out=RUN/'study';out.mkdir(exist_ok=False)
binary=Path(json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['generic']['binary'])
subprocess.run(['conda','run','-n','kaggriculture','python',str(EXP/'scripts/freeze_arena_inputs.py'),str(binary),str(out/'freeze')],check=True)
frozen=json.loads((out/'freeze/FROZEN.json').read_text())['files_sha256']
build=json.loads((binary.parent/'build.json').read_text())['command']
command=[str(RUN/'source/study.cpp') if value==str(EXP/'src/arena.cpp') else value for value in build]
command[-1]=str(out/'study')
assert str(RUN/'source/study.cpp') in command
for p in [RUN/'source/study.cpp',Path(__file__)]:frozen[str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
with (out/'build.log').open('x') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
variants=['animal_groups_m0','animal_groups_m1','animal_groups_m2','animal_groups_cow1','animal_groups_cow3','animal_groups_sheep12','animal_groups_sheep3']
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','junghoon_wool_sales','king_rc4','yusuke_sep08_m2','investment_context_guarded_001_best','pass']
jobs=[(a,b) for b in opponents for a in variants]
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'build_command':command,
    'source_sha256':frozen,'jobs':jobs,'seed_start':1000,'seeds':64,'seats':'both','games':len(jobs)*128,
    'scope':'Exposed discovery with exact registered-adapter controls, production/labor profiles and runtime selection/guard diagnostics; not a promotion.'},indent=2)+'\n')
def run(job):
    a,b=job;path=out/f'{a}_vs_{b}.json'
    cmd=['conda','run','-n','kaggriculture',str(out/'study'),'--a',a,'--b',b,'--games','64','--seed-start','1000',
        '--seat-mode','both','--threads','4','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('x') as log:subprocess.run(cmd,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
    print(a,'vs',b,'complete',flush=True);return cmd
with ThreadPoolExecutor(max_workers=2) as pool:commands=list(pool.map(run,jobs))
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in frozen.items())
(out/'EXECUTION.json').write_text(json.dumps({'commands':commands,'sources_unchanged':True,'games':len(jobs)*128,'registered_control_games':len(jobs)*8},indent=2)+'\n')
