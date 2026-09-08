"""Fresh paired factor comparison of composition selection and sale priority."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
assert json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())['games']==32
out=RUN/'fresh_2340000';out.mkdir(exist_ok=False)
binary=json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['generic']['binary']
subprocess.run(['conda','run','-n','kaggriculture','python',str(EXP/'scripts/freeze_arena_inputs.py'),binary,str(out/'freeze')],check=True)
frozen=json.loads((out/'freeze/FROZEN.json').read_text())['files_sha256']
agents=['empty_sale_slots_m2','premium_sales_s216','animal_groups_m1','animal_groups_m2','animal_premium_m1','animal_premium_m2']
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','junghoon_wool_sales','king_rc4','yusuke_sep08_m2','ahmed_v24','investment_context_guarded_001_best','pass']
jobs=[{'a':a,'b':b,'seeds':256,'seed_start':2340000,'native':False} for b in opponents for a in agents]
jobs += [{'a':a,'b':b,'seeds':64,'seed_start':2344000,'native':True} for b in ['empty_sale_slots_m2','teammate_shoprouter','yusuke_sep08_m2','pass'] for a in agents]
count=sum(j['seeds']*2 for j in jobs)
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'jobs':jobs,'seats':'both','games':count,
    'selection':'Fresh factor comparison: parent, sale priority alone, two animal-market modes, and their combinations. Exclude direct-parent/self/PASS from neutral win group; report all regressions, production, labor and tails.',
    'scope':'Independent selection evidence, not automatic promotion. Unexpected-weed repair and expanded league checks remain required; Final900000 unused.'},indent=2)+'\n')
def run(job):
    prefix='native_' if job['native'] else '';path=out/f"{prefix}{job['a']}_vs_{job['b']}.json"
    cmd=['conda','run','-n','kaggriculture',binary,'--a',job['a'],'--b',job['b'],'--games',str(job['seeds']),
        '--seed-start',str(job['seed_start']),'--seat-mode','both','--threads','4','--validate','--profile','--output',str(path)]
    if job['native']:cmd.append('--native-shops')
    with path.with_suffix('.log').open('x') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    print(path.stem,'complete',flush=True);return cmd
with ThreadPoolExecutor(max_workers=2) as pool:commands=list(pool.map(run,jobs))
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in frozen.items())
(out/'EXECUTION.json').write_text(json.dumps({'commands':commands,'sources_unchanged':True,'games':count},indent=2)+'\n')
