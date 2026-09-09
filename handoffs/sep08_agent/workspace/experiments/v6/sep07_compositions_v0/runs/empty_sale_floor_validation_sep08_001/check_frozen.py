"""Freeze and rebuild the isolated pair, then compare complete native records."""
from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
candidate='empty_sale_floor_m1'
binary=Path(json.loads((RUN/'CHECK_BINARIES.json').read_text())['pair'])
frozen=RUN/'frozen'
cmd=['conda','run','-n','kaggriculture','python',str(EXP/'scripts/freeze_arena_inputs.py'),str(binary),str(frozen)]
subprocess.run(cmd,check=True)
spec=json.loads((frozen/'FROZEN.json').read_text())
with (frozen/'rebuild.log').open('w') as log:subprocess.run(spec['rebuild_command'],stdout=log,stderr=subprocess.STDOUT,check=True)
output=frozen/'rebuilt_vs_public_router_v52.json'
cmd=['conda','run','-n','kaggriculture',str(frozen/'arena_rebuilt'),'--a',candidate,'--b','public_router_v52','--games','32','--seed-start','2103000','--seat-mode','both','--threads','4','--profile','--validate','--native-shops','--output',str(output)]
subprocess.run(cmd,check=True)
actual=json.loads(output.read_text())['games']
expected=json.loads((RUN/f'checks/generic_t1_native1_{candidate}_vs_public_router_v52.json').read_text())['games']
control=json.loads((RUN/'checks/generic_t1_native1_observed_sale_lead_start_216_vs_public_router_v52.json').read_text())['games']
assert actual==expected and len(actual)==64
changed=sum(a['action_hash']!=b['action_hash'] for a,b in zip(actual,control))
assert changed
report={'frozen_full_records_equal':True,'frozen_games':64,'active_branch_changed_games':changed,'opponent':'public_router_v52','dependencies':len(spec['files_sha256']),'command':cmd,'scope':'Isolated candidate/opponent pair includes actual compiler-MM dependencies; rebuilt full native records equal generic checks.'}
(RUN/'FINAL_AUDITS.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
