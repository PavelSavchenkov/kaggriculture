from pathlib import Path
import json,subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
build=json.loads((EXP/'runs/cold_renewal_sep08_001/FRESH_BINARIES.json').read_text())['generic']
manifest=Path(build['binary']).parent/'build.json'
command=json.loads(manifest.read_text())['command']
command=[str(RUN/'prefix.cpp') if v==str(EXP/'src/arena.cpp') else v for v in command]
command[command.index('-o')+1]=str(RUN/'prefix_check')
result=subprocess.run(command,capture_output=True,text=True)
(RUN/'prefix_build.log').write_text(result.stdout+result.stderr);result.check_returncode()
execute=['conda','run','-n','kaggriculture',str(RUN/'prefix_check'),str(RUN/'PREFIX.json')]
result=subprocess.run(execute,capture_output=True,text=True)
(RUN/'prefix_run.log').write_text(result.stdout+result.stderr)
(RUN/'PREFIX_COMMANDS.json').write_text(json.dumps([command,execute],indent=2)+'\n')
result.check_returncode()
report=json.loads((RUN/'PREFIX.json').read_text())
assert report['total']==report['equal_through_step143']==1632
assert all(r['step']==144 and len(r['shops'])==2 for r in report['prefixes'])
print('1632 common legal prefixes exact through step143; only first two shops visible at selection.',flush=True)
