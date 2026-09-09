"""Freeze two full public courses and exact pre-branch states as offline data."""
import csv
import hashlib
import json
import sys
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
sys.path.insert(0, str(EXP / 'scripts'))
from export_animal_replay_findings import actions, difference, state_at
from generate_public_routes import triple


def dump(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n')


def main():
    out=RUN/'sheep_expansion';out.mkdir(exist_ok=True)
    metadata=json.loads((RUN/'animal_decisions/games.json').read_text())
    sources=[next(r for r in metadata if r['episode']==ep and r['team']=='自己找差距')for ep in [106458227,106463571]]
    states=[];courses=[]
    for label,source in zip(['small','expansion'],sources):
        folder=out/label;folder.mkdir(exist_ok=True);episode,seat=source['episode'],source['seat']
        path=EXP/'replays'/f'episode-{episode}-replay.json';replay=json.loads(path.read_text());raw=actions(episode,seat)
        normalized=[]
        for t,action in enumerate(raw):
            units=[triple(a)for a in [action['farmer'],*action['hands']]];n=1+len(replay['steps'][t][0]['observation']['farms'][seat]['hands'])
            normalized.append({'units':(units+[[0,0,0]]*n)[:n],'market':[triple(a,True)for a in action['market']]})
        assert len(normalized)==719
        dump(folder/'raw_actions_719.json',raw);dump(folder/'normalized_actions_719.json',normalized)
        state=state_at(episode,seat,288);dump(folder/'state_before_288.json',state);states.append(state);courses.append(normalized)
        for name in ['animal_instances','crop_instances','animal_days','crop_days','transactions','unit_events','daily_product','tile_transitions']:
            rows=[r for r in csv.DictReader((RUN/f'{name}.csv').open())if int(r['episode_id'])==episode and r['team']==source['team']]
            with (folder/f'{name}.csv').open('w')as f:
                writer=csv.DictWriter(f,fieldnames=rows[0]);writer.writeheader();writer.writerows(rows)
        dump(folder/'IMPORT.json',{'source':source,'replay_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'normalization':'Existing triple encoder; active worker count only. No original private branch formula recovered.','artifacts':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in folder.iterdir()if p.is_file()}})
    a,b=states
    record={'sources':sources,'branch_step':288,'day':12,'raw_prefix_equal':next((t for t,(a,b)in enumerate(zip(actions(sources[0]['episode'],sources[0]['seat']),actions(sources[1]['episode'],sources[1]['seat'])))if a!=b),719),'normalized_prefix_equal':next((t for t,(a,b)in enumerate(zip(*courses))if a!=b),719),'own_farm_differences':difference(a['own_farm'],b['own_farm']),'own_private_differences':difference(a['own_private'],b['own_private']),'own_private_ordered_equal':a['own_private_ordered']==b['own_private_ordered'],'public_market_differences':difference(a['public_market'],b['public_market']),'shops_a':a['town'],'shops_b':b['town'],'hypotheses':['More already revealed Yarn demand may justify expansion of the full sheep family and conversion of crop tiles.','Existing own output, public market inventory, funding and rival flow may modify a simple shop threshold.','An observed branch is not proof of one particular private donor formula.']}
    dump(out/'COMPARISON.json',record)
    print(json.dumps({k:v for k,v in record.items()if k not in ['sources','public_market_differences']},ensure_ascii=False),flush=True)


if __name__=='__main__':main()
