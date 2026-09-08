"""Test premium sale order changes against the new counter and older diverse opponents."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
assert (RUN/'OPERATIONAL_CHECKS.json').exists()
assert json.loads((RUN/'SOURCE_PARITY.json').read_text())['cases']==4096
out=RUN/'discovery';out.mkdir(exist_ok=False)
binary=json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['generic']['binary']
control=out/'control.json'
cmd=['conda','run','-n','kaggriculture',binary,'--a','premium_sales_m0','--b','public_router',
    '--games','8','--seed-start','2330000','--seat-mode','both','--threads','4','--validate','--profile','--output',str(control)]
with (out/'control.log').open('x') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
actual=json.loads(control.read_text())['games']
prior=json.loads((EXP/'runs/yusuke_port_sep08_001/fresh_2330000/empty_sale_slots_m2_vs_public_router.json').read_text())['games']
expected=[g for g in prior if g['seed']<2330008]
assert actual==expected
(out/'CONTROL.json').write_text(json.dumps({'games':16,'complete_records_equal':True,'command':cmd},indent=2)+'\n')
subprocess.run(['conda','run','-n','kaggriculture','python',str(EXP/'scripts/freeze_arena_inputs.py'),binary,str(out/'freeze')],check=True)
frozen=json.loads((out/'freeze/FROZEN.json').read_text())['files_sha256']
variants=['premium_sales_s144','premium_sales_s216','empty_sale_slots_m2','ahmed_v24','ahmed_v23']
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23',
    'junghoon_wool_sales','king_rc4','yusuke_sep08_m2','ahmed_v24','pass','investment_context_guarded_001_best']
jobs=[(a,b) for b in opponents for a in variants]
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'jobs':jobs,'seed_start':1000,'seeds':64,'seats':'both','games':7040,
    'scope':'Exposed paired discovery. Preserve all opponent regressions, own cash/margin/tails and exact control; direct counter improvement alone is insufficient.'},indent=2)+'\n')
def run(job):
    a,b=job;path=out/f'{a}_vs_{b}.json'
    cmd=['conda','run','-n','kaggriculture',binary,'--a',a,'--b',b,'--games','64','--seed-start','1000',
        '--seat-mode','both','--threads','4','--validate','--profile','--output',str(path)]
    with path.with_suffix('.log').open('x') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    print(a,'vs',b,'complete',flush=True);return cmd
with ThreadPoolExecutor(max_workers=2) as pool:commands=list(pool.map(run,jobs))
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in frozen.items())
(out/'EXECUTION.json').write_text(json.dumps({'commands':commands,'sources_unchanged':True,'games':7040},indent=2)+'\n')
