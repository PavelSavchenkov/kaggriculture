from pathlib import Path
import json,subprocess
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
built=Path(json.loads((RUN/'BUILD.json').read_text())['binary'])
command=json.loads((built.parent/'build.json').read_text())['command']
command[command.index(str(EXP/'src/arena.cpp'))]=str(RUN/'trace.cpp')
command[command.index('-o')+1]=str(RUN/'trace')
with (RUN/'trace_build.log').open('w') as output:subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,check=True)
(RUN/'TRACE_BUILD.json').write_text(json.dumps(command,indent=2)+'\n')
subprocess.run(['conda','run','-n','kaggriculture',str(RUN/'trace'),str(RUN/'traces')],check=True)
print('Full diagnostic traces complete.',flush=True)
