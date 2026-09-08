from pathlib import Path
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
binary=RUN/'forecast_probe'
command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-march=native','-DNDEBUG','-fno-exceptions','-fno-rtti','-flto','-I',str(ROOT),
         str(RUN/'forecast_probe.cpp'),str(EXP/'league/public_router_v52/source/agent.cpp'),str(RUN/'parent_source/league/top_replay_library/source/agent.cpp'),
         str(RUN/'parent_source/league/public_router/source/agent.cpp'),str(EXP/'league/top_replay_library/source/agent.cpp'),str(EXP/'league/public_router/source/agent.cpp'),'-o',str(binary)]
result=subprocess.run(command,capture_output=True,text=True)
(RUN/'probe_build.log').write_text(result.stdout+result.stderr);result.check_returncode()
run=['conda','run','-n','kaggriculture',str(binary),str(RUN/'FORECAST_DIAGNOSTICS.json')]
subprocess.run(run,check=True)
(RUN/'PROBE_COMMANDS.json').write_text(json.dumps([command,run],indent=2)+'\n')
for row in json.loads((RUN/'FORECAST_DIAGNOSTICS.json').read_text()):
    print(row,'forecast_accuracy',row['matched']/row['forecasts'],flush=True)
