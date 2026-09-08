from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
build = json.loads((RUN/'BUILD.json').read_text())
command = json.loads((Path(build['binary']).parent/'build.json').read_text())['command']
command = [str(RUN/'probe.cpp') if value==str(EXP/'src/arena.cpp') else value for value in command]
command[command.index('-o')+1] = str(RUN/'probe')
result = subprocess.run(command,capture_output=True,text=True)
(RUN/'probe_build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
execute = ['conda','run','-n','kaggriculture',str(RUN/'probe'),str(RUN/'OBSERVATIONS.json')]
result = subprocess.run(execute,capture_output=True,text=True)
(RUN/'probe_run.log').write_text(result.stdout+result.stderr)
(RUN/'PROBE_COMMANDS.json').write_text(json.dumps([command,execute],indent=2)+'\n')
result.check_returncode()
report = json.loads((RUN/'OBSERVATIONS.json').read_text())
assert report['common_prefixes']==len(report['observations'])==1152
assert all(len(row['shops'])==4 for row in report['observations'])
print('1152 exact common prefixes and legal day12 observations exported.',flush=True)
