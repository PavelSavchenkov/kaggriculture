"""Compile the same diagnostic against each family's frozen C++ dependency set."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from branch_inputs import FAMILIES

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]


def main():
    parser=argparse.ArgumentParser();parser.add_argument('family',choices=list(FAMILIES));parser.add_argument('--residuals',action='store_true');args=parser.parse_args();cfg=FAMILIES[args.family]
    old=EXP/'runs'/cfg['source']/'build/generic/BUILD.json';manifest=json.loads(old.read_text());command=manifest['binaries'][1]['command'].copy()
    kind='residuals'if args.residuals else'forecast'
    (RUN/'build').mkdir(exist_ok=True);source=RUN/'source'/('residuals.cpp'if args.residuals else'branch_forecast.cpp');binary=RUN/'build'/f'{args.family}_{kind}'
    command=[str(source)if x.endswith('/source/diagnostics.cpp')else x for x in command];command[-1]=str(binary)
    command += [f'-DPREFIX_STEP={cfg["prefix"]}',f'-DFIXED_POLICY="{cfg["branches"][0]}"',f'-DFERT_CENTER={int(args.family=="atakan")}']
    if args.family=='atakan':command.append('-DATAKAN')
    with (RUN/f'build/{args.family}_{kind}_compile.log').open('w')as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    run=['conda','run','-n','kaggriculture',str(binary),str(RUN/f'{args.family}_inputs.txt'),str(RUN/f'{args.family}_{"residuals"if args.residuals else"predictions"}.json')]
    subprocess.run(run,check=True)
    (RUN/f'build/{args.family}_{"residuals_"if args.residuals else""}BUILD.json').write_text(json.dumps({'commands':[command,run],'frozen_dependency_manifest':str(old),'frozen_manifest_sha256':hashlib.sha256(old.read_bytes()).hexdigest(),'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in [source,RUN/'source/crop_forecast.hpp',binary]},'scope':'Offline observation-only forecast diagnostic. No policy package, source catalog or deployment changed.'},indent=2)+'\n')


if __name__=='__main__':main()
