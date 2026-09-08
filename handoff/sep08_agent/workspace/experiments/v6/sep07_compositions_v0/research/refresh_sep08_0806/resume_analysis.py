from datetime import datetime,timezone
from pathlib import Path
import csv
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
commands=[]
jobs=[('normalize_replay_names.py',['--research-dir',str(RUN),'--previous-dir',str(EXP/'research/refresh_sep08_0708')],'replay_names.log'),
    ('analyze_replays.py',['--research-dir',str(RUN)],'replay_analysis_resumed.log'),
    ('review.py',['--research-dir',str(RUN)],'invariant_analysis.log'),
    ('refresh_metadata.py',['--research-dir',str(RUN)],'metadata.log')]
for script,args,log in jobs:
    command=['conda','run','--no-capture-output','-n','kaggriculture','python',str(EXP/'scripts'/script),*args]
    with (RUN/log).open('w') as f:subprocess.run(command,stdout=f,stderr=subprocess.STDOUT,check=True)
    commands.append({'command':command,'log':log})
rows=list(csv.DictReader((RUN/'top_replay_manifest.csv').open()))
paths=[p for p in RUN.rglob('*') if p.is_file()]
paths+=sorted({EXP/'replays'/f"episode-{r['episode_id']}-replay.json" for r in rows})
paths+=[EXP/'scripts'/n for n in ['refresh_top_evidence.py','pull_replays.py','normalize_replay_names.py','analyze_replays.py','review.py','refresh_metadata.py']]
report={'completed_utc':datetime.now(timezone.utc).isoformat(),'commands':commands,'player_games':len(rows),
    'raw_unique_replays':len({r['episode_id'] for r in rows}),
    'recovery':'Original refresh7865 terminated in name resolution after successful downloads. Original failed log/manifest preserved; only normalization and downstream analysis rerun. Stable team16665237/submission56058327 maps Ad Space Available to historical get some fries where needed.',
    'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
(RUN/'REFRESH.json').write_text(json.dumps(report,indent=2)+'\n')
print('Completed renamed-team refresh:',report['player_games'],report['raw_unique_replays'],flush=True)
