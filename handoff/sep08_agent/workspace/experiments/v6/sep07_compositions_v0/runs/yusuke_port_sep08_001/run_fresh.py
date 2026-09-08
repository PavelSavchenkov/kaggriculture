"""Independent new-opponent confirmation with paired strongest-agent controls."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import hashlib
import json
import subprocess
import time

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
assert (RUN/'RESULTS.json').exists()
out=RUN/'fresh_2330000';out.mkdir(exist_ok=False)
binary=json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['generic']['binary']
subprocess.run(['conda','run','-n','kaggriculture','python',str(EXP/'scripts/freeze_arena_inputs.py'),binary,str(out/'freeze')],check=True)
frozen=json.loads((out/'freeze/FROZEN.json').read_text())['files_sha256']
variants=['yusuke_sep08_m1','yusuke_sep08_m2','yusuke_sep08_m3','empty_sale_slots_m2']
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','junghoon_wool_sales','king_rc4']
jobs=[(a,b) for b in opponents for a in variants]
protocol={'created_utc':datetime.now(timezone.utc).isoformat(),'seed_start':2330000,'seeds':512,'seats':'both','jobs':jobs,
    'scope':'Independent confirmation of a strong public opponent and paired comparison with the current best. Seven-opponent panel; not a full33-opponent promotion gate.',
    'selection':'Compare league utility, own cash, margin, tails and opponent-specific regressions. Preserve the current reference until full promotion evidence exists.',
    'observed_discovery':'Full source128/128against current; base116/128. Day27mean gain with utility regression; local preservation amendment negative. Final900000 untouched.'}
(out/'PROTOCOL.json').write_text(json.dumps(protocol,indent=2)+'\n')
def run(job):
    a,b=job;path=out/f'{a}_vs_{b}.json'
    cmd=['conda','run','-n','kaggriculture',binary,'--a',a,'--b',b,'--games','512','--seed-start','2330000',
        '--seat-mode','both','--threads','4','--budget-expansions','100000','--validate','--profile','--output',str(path)]
    start=time.monotonic()
    with path.with_suffix('.log').open('x') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    print(a,'vs',b,'complete',flush=True)
    return {'command':cmd,'seconds':time.monotonic()-start}
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(run,jobs))
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in frozen.items())
(out/'EXECUTION.json').write_text(json.dumps({'commands':records,'sources_unchanged':True,'games':28672},indent=2)+'\n')
