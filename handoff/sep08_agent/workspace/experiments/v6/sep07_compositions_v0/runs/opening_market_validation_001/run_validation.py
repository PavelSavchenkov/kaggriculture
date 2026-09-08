from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]


def main():
    spec=json.loads((RUN/'PREREGISTERED.json').read_text())
    binary=Path(next(x for x in reversed((RUN/'build.log').read_text().splitlines()) if x.startswith(str(EXP/'build'))))
    commands=[]

    def execute(agent,opponent,directory,seeds,seed,profile=False):
        path=directory/f'{agent}_vs_{opponent}.json';assert not path.exists()
        cmd=['conda','run','-n','kaggriculture',str(binary),'--a',agent,'--b',opponent,
             '--games',str(seeds),'--seed-start',str(seed),'--seat-mode','both','--threads','4',
             '--validate','--output',str(path)]
        if profile:cmd.append('--profile')
        commands.append(cmd)
        with path.with_suffix('.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
        return json.loads(path.read_text())

    parity=RUN/'wrapper_parity';parity.mkdir();checked=0
    source=EXP/'runs/opening_market_search_001'
    for agent,choice in [(spec['candidate'],'q32_b13_m2'),(spec['diagnostic_control'],'q81_b13_m2')]:
        for opponent in spec['opponents']:
            expected=next(p for p in [source/'discovery'/f'{choice}_vs_{opponent}.json',source/'extended_discovery'/f'{choice}_vs_{opponent}.json'] if p.exists())
            result=execute(agent,opponent,parity,64,1000)
            assert result['games']==json.loads(expected.read_text())['games'],(agent,opponent)
            checked+=len(result['games'])
    (RUN/'WRAPPER_PARITY.json').write_text(json.dumps({'full_records_exact':checked,'scope':'Both normal C++ agent packages match the typed constructor search across20opponents.'},indent=2)+'\n')
    print('Wrapper parity',checked,flush=True)
    fresh=RUN/'fresh';fresh.mkdir()
    with ThreadPoolExecutor(max_workers=2) as pool:
        futures=[pool.submit(execute,agent,opponent,fresh,spec['seeds'],spec['seed_start'])
                 for agent in [spec['candidate'],spec['baseline'],spec['diagnostic_control']] for opponent in spec['opponents']]
        for f in futures:f.result()
    profiles=RUN/'profiles';profiles.mkdir()
    for opponent in ['bohann_opening_v1','teammate_shoprouter','king_rc4','public_router_v5','public_sixday']:
        for agent in [spec['candidate'],spec['baseline'],spec['diagnostic_control']]:execute(agent,opponent,profiles,32,1000,True)
    (RUN/'COMMANDS.json').write_text(json.dumps({'binary':str(binary),'commands':commands},indent=2)+'\n')
    print('Fresh and profile matrices complete',flush=True)


if __name__=='__main__':main()
