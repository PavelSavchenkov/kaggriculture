"""Common-seed full C++ games and independent observation diagnostics."""
import argparse
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from generate import NAMES

RUN=Path(__file__).resolve().parent
RIVALS=['investment_context_guarded_001_best','teammate_shoprouter','public_router_v5','king_rc4','public_router']


def run(job):
    name,a,b,seeds,kind,seed=job;out=RUN/'results';out.mkdir(exist_ok=True)
    mode='debug'if kind=='debug'else'masked'if kind=='masked'else'generic';binary='diagnostics'if kind=='trace'else'arena'
    command=['conda','run','-n','kaggriculture',str(RUN/'build'/mode/binary),'--a',a,'--b',b,'--games',str(seeds),'--seed-start',str(seed),'--seat-mode','both','--threads','1'if kind in ['debug','masked','thread']else'4','--validate','--output',str(out/f'{name}.json')]
    if kind=='profile':command.append('--profile')
    if kind=='native':command.append('--native-shops')
    with (out/f'{name}.log').open('w')as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    return {'name':name,'command':command}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('phase',choices=['discovery','trace','profile','checks']);args=parser.parse_args()
    if args.phase in ['discovery','trace','profile']:
        names=NAMES if args.phase!='profile'else NAMES[:2];kind=''if args.phase=='discovery'else args.phase;count=8 if kind=='profile'else 32
        jobs=[(f'{args.phase}_{a}_vs_{b}',a,b,count,kind,1000)for a in names for b in RIVALS]
    else:
        names=['sheep_fixed_small','sheep_fixed_expansion','sheep_yarn2','sheep_value_margin_s64']
        jobs=[(f'{a}_pass128',a,'pass',64,'',1000)for a in names]+[(f'{a}_self16',a,a,8,'',1000)for a in names]
        jobs += [(f'operational_{kind}16','sheep_yarn2','sheep_value_margin_s64',8,kind,1000)for kind in ['','thread','masked','debug']]
    with ThreadPoolExecutor(max_workers=4)as pool:records=list(pool.map(run,jobs))
    (RUN/f'{args.phase.upper()}_COMMANDS.json').write_text(json.dumps(records,indent=2)+'\n');print(args.phase,len(jobs),'batches',flush=True)


if __name__=='__main__':main()
