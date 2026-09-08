from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
report = json.loads((RUN/'CHECKS.json').read_text())
assert not report['differences'] and report['cases']==216
command = report['commands'][0]
command[command.index(str(RUN/'check.cpp'))]=str(RUN/'benchmark.cpp')
command[command.index('-o')+1]=str(RUN/'benchmark')
with (RUN/'benchmark_build.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
run=['conda','run','-n','kaggriculture',str(RUN/'benchmark'),str(RUN/'cases.txt')]
result=subprocess.run(run,capture_output=True,text=True,check=True)
benchmark=json.loads(result.stdout)
benchmark['commands']=[command,run]
benchmark['scope']='Single C++ process, 30 repeats of216 verified contexts, wall time under concurrent audit load. Excludes conda startup, source-oracle and build time. No GPU used.'
(RUN/'BENCHMARK.json').write_text(json.dumps(benchmark,indent=2)+'\n')
print({k:v for k,v in benchmark.items() if k!='commands'})
