"""Compile exact local C++ crop-event tracing against the frozen reference."""
import hashlib
import json
import subprocess
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]


def main():
    build=RUN/'build';build.mkdir(exist_ok=True)
    manifest_path=EXP/'runs/atakan_sampled_shops_001/build/generic/BUILD.json'
    manifest=json.loads(manifest_path.read_text());command=manifest['binaries'][1]['command'].copy()
    command=[str(RUN/'local_events.cpp') if p.endswith('/source/diagnostics.cpp') else p for p in command];command[-1]=str(build/'local_events')
    with (build/'compile.log').open('w')as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT)
    run=['conda','run','-n','kaggriculture',str(build/'local_events'),'--seed-start','1000','--games','4','--output',str(RUN/'local_events.json')]
    subprocess.run(run,check=True)
    original=json.loads((EXP/'results/investment_context_checks/investment_context_guarded_001_best_profile64.json').read_text())['games'];lookup={(g['seed'],g['seat']):g for g in original}
    result=json.loads((RUN/'local_events.json').read_text())
    for game in result:
        old=lookup[game['seed'],game['seat']]
        for key in ['cash','opponent_cash','action_hash','opponent_action_hash']:assert str(old[key])==str(game[key])
        for product in range(5):assert sum(e[7]for e in game['events']if e[2]==10 and e[3]==product)==old['produced'][product]
        assert sum(e[2]==11 for e in game['events'])==old['profile']['successful'][11]
    (RUN/'LOCAL_TRACE_VALIDATION.json').write_text(json.dumps({'games':len(result),'cash_hash_and_crop_output_parity':True,'commands':[command,run],
        'frozen_dependency_manifest':str(manifest_path),'frozen_dependency_manifest_sha256':hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
        'binary_sha256':hashlib.sha256((build/'local_events').read_bytes()).hexdigest()},indent=2)+'\n')
    print(f'{len(result)} local crop traces match exact original games.')


if __name__=='__main__':main()
