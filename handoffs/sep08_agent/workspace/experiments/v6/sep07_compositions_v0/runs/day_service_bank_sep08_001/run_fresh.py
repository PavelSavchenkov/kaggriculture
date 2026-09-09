from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,subprocess
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
actors=['service_bank_p362_m0','service_bank_p362_m2']
opponents=['empty_sale_slots_m2','public_router','joint_routes_p362_m0','day_program_p362_m3','pass']
protocol={'created_utc':datetime.now(timezone.utc).isoformat(),'actors':actors,'opponents':opponents,'seed_start':2300000,'seeds':128,'seat_mode':'both','games':2560,'selection_scope':'This can establish a better p362 compiler continuation only, never promotion to strongest reference.','gates':['Mean cash gain nonnegative against each opponent.','Positive95%seed-cluster bootstrap lower bound for equally weighted five-opponent mean cash gain.','Direct old-program-parent mean margin positive and tie-half utility at least0.5.','No increase in mean unit faults against any opponent.','Operational checks, coverage/full-game equality and input hashes unchanged.'],'report_also':['Mean paired margin/utility, production, sold, discards, hires/cost, tail outcomes and activation limits.'],'final_pool_900000':'untouched'}
with (RUN/'FRESH_PROTOCOL.json').open('x') as f:json.dump(protocol,f,indent=2)
roots=[EXP/'runs',EXP/'league',EXP/'include',ROOT/'fast_game_engine',ROOT/'agents/common/api',ROOT/'agents/common/runtime']
inputs={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for root in roots for p in root.rglob('*') if p.is_file() and p.suffix in {'.cpp','.hpp','.inc','.h'}}
(RUN/'FRESH_INPUTS.json').write_text(json.dumps(inputs,indent=2)+'\n')
binary=json.loads((RUN/'BUILD.json').read_text())['binary']
out=RUN/'fresh';out.mkdir(exist_ok=False)
def run(job):
 a,b=job;path=out/f'{a}_vs_{b}.json'
 command=['conda','run','-n','kaggriculture',binary,'--a',a,'--b',b,'--games','128','--seed-start','2300000','--seat-mode','both','--threads','4','--budget-expansions','100000','--validate','--profile','--output',str(path)]
 with path.with_suffix('.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
 d=json.loads(path.read_text());print(a,b,'cash',d['mean_cash'],'margin',d['mean_margin'],flush=True);return command
with ThreadPoolExecutor(max_workers=2) as pool:commands=list(pool.map(run,[(a,b) for b in opponents for a in actors]))
for path,value in inputs.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==value,path
(RUN/'FRESH_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
print('2560 fresh compiler games complete; all original input hashes unchanged.',flush=True)
