"""Paired exact C++ gameplay and separate guard diagnostics."""
import argparse
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from format_packages import NAMES

RUN=Path(__file__).resolve().parent
RIVALS=['crop_value_m2_t4','teammate_shoprouter','public_router_v5','king_rc4','public_router']


def execute(job):
    name,a,b,count,seed,kind=job;out=RUN/'results';out.mkdir(exist_ok=True)
    build='policies_debug'if kind=='debug'else'policies';binary='diagnostics'if kind=='trace'else'arena'
    command=['conda','run','-n','kaggriculture',str(RUN/'build'/build/binary),'--a',a,'--b',b,
             '--games',str(count),'--seed-start',str(seed),'--seat-mode','both','--threads','1'if kind in ['thread','debug','trace']else'3','--validate','--output',str(out/f'{name}.json')]
    if kind=='profile':command.append('--profile')
    if kind=='native':command.append('--native-shops')
    with (out/f'{name}.log').open('w')as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    return {'name':name,'command':command}


def main():
    parser=argparse.ArgumentParser();parser.add_argument('phase',choices=['discovery','trace','checks','fresh','native','fresh_trace']);args=parser.parse_args()
    if args.phase=='discovery':jobs=[(f'discovery_{a}_vs_{b}',a,b,32,1000,'profile')for a in NAMES+['crop_value_m2_t4']for b in RIVALS]
    elif args.phase=='trace':jobs=[(f'trace_{a}_vs_{b}',a,b,32,1000,'trace')for a in NAMES for b in RIVALS]
    elif args.phase=='fresh':jobs=[(f'fresh_{a}_vs_{b}',a,b,256,1700000,'')for a in ['wheat_one_fert','wheat_one_plain','wheat_three_fert','crop_value_m2_t4']for b in RIVALS]
    elif args.phase=='native':jobs=[(f'native_{a}_vs_{b}',a,b,64,1701000,'native')for a in ['wheat_one_fert','wheat_one_plain','wheat_three_fert','crop_value_m2_t4']for b in RIVALS]
    elif args.phase=='fresh_trace':jobs=[(f'fresh_trace_wheat_one_fert_vs_{b}','wheat_one_fert',b,256,1700000,'trace')for b in RIVALS]
    else:
        jobs=[(f'{a}_pass128',a,'pass',64,1000,'')for a in NAMES[:4]]+[(f'{a}_self16',a,a,8,1000,'')for a in NAMES[:4]]
        jobs += [(f'operational_{kind or "generic"}16','wheat_one_fert','wheat_three_fert',8,1000,kind)for kind in ['','thread','debug']]
    with ThreadPoolExecutor(max_workers=4)as pool:commands=list(pool.map(execute,jobs))
    (RUN/f'{args.phase.upper()}_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n');print(args.phase,len(commands),'batches')


if __name__=='__main__':main()
