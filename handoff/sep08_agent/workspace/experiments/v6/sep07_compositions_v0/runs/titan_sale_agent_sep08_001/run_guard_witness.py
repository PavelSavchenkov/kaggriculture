from pathlib import Path
import json,subprocess
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
built=Path(json.loads((RUN/'BUILD.json').read_text())['binary'])
command=json.loads((built.parent/'build.json').read_text())['command']
command[command.index(str(EXP/'src/arena.cpp'))]=str(RUN/'guard_witness.cpp')
command[command.index('-o')+1]=str(RUN/'guard_witness')
with (RUN/'guard_build.log').open('w') as output:subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,check=True)
(RUN/'GUARD_BUILD.json').write_text(json.dumps(command,indent=2)+'\n')
subprocess.run(['conda','run','-n','kaggriculture',str(RUN/'guard_witness'),str(RUN/'guard_witnesses')],check=True)
print('Full diagnostic traces complete.',flush=True)
