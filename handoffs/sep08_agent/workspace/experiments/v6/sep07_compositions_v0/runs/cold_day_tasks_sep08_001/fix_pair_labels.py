"""Preserve and correct metadata from the initial unlabeled static-pair call."""
from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
path=RUN/'operations/pair.json'
original=json.loads(path.read_text())
assert original['agent_a']=='teammate_shoprouter' and original['agent_b']=='pass'
spec=json.loads((RUN/'OPERATIONAL_BINARIES.json').read_text())['pair']
assert spec['command'][-3:]==['--pair','cold_day_tasks_full','public_router']
saved=RUN/'operations/pair_original_labels.json'
assert not saved.exists()
path.rename(saved)
command=['conda','run','-n','kaggriculture',spec['binary'],'--a','cold_day_tasks_full','--b','public_router',
    '--games','2','--seed-start','1000','--seat-mode','both','--threads','2','--budget-expansions','100000',
    '--validate','--profile','--output',str(path)]
subprocess.run(command,check=True)
corrected=json.loads(path.read_text())
assert corrected['games']==original['games']
assert corrected['agent_a']=='cold_day_tasks_full' and corrected['agent_b']=='public_router'
report_path=RUN/'OPERATIONAL_CHECKS.json'
report=json.loads(report_path.read_text())
report['games']+=len(corrected['games'])
report['pair_label_correction']={'original':str(saved.relative_to(RUN)),'original_labels':[original['agent_a'],original['agent_b']],
    'correct_labels':[corrected['agent_a'],corrected['agent_b']],'complete_records_equal':len(corrected['games']),'command':command}
report_path.write_text(json.dumps(report,indent=2)+'\n')
print('Pair labels corrected; all four complete records remain identical.')
