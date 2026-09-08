"""Prepare fixed own plans and exact scoring outcomes from frozen panels."""
import hashlib
import json
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
FAMILIES={
    'atakan':{'source':'atakan_portfolio_001','branches':['atakan_cow','atakan_sheep','atakan_goose'],'prefix':226,'rivals':['animal_adaptive_r1_c0_b0','teammate_shoprouter','public_router_v5','king_rc4','public_router']},
    'sheep':{'source':'sheep_expansion_portfolio_001','branches':['sheep_fixed_small','sheep_fixed_expansion'],'prefix':288,'rivals':['investment_context_guarded_001_best','teammate_shoprouter','public_router_v5','king_rc4','public_router']},
}


def flatten(value):
    for x in value:
        if isinstance(x,list):yield from flatten(x)
        else:yield str(x)


def main():
    lineage={}
    for family,cfg in FAMILIES.items():
        source=EXP/'runs'/cfg['source'];hashes={};expected=[];inputs=[]
        def load(relative):
            p=source/relative;hashes[str(p.relative_to(EXP))]=hashlib.sha256(p.read_bytes()).hexdigest();return json.loads(p.read_text())
        models=load('flow_models.json')
        for rival in cfg['rivals']:
            panels=[load(f"results/{'discovery_'if family=='sheep'else''}{n}_vs_{rival}.json")['games']for n in cfg['branches']]
            trace=load(f"results/trace_{cfg['branches'][0]}_vs_{rival}.json"if family=='sheep'else f'results/trace_{rival}.json')
            for index,t in enumerate(trace):
                games=[panel[index]for panel in panels];assert all((g['seed'],g['seat'])==(t['seed'],t['seat'])for g in games)
                assert all(g['shops']==games[0]['shops']for g in games)
                inputs.append([rival,t['seed'],t['seat'],games[0]['shops']])
                expected.append({'rival':rival,'seed':t['seed'],'seat':t['seat'],'own_cash':t[f"cash_before{cfg['prefix']}"],'rival_cash':t[f"rival_cash_before{cfg['prefix']}"],'physical':t[f"physical_before{cfg['prefix']}"],'own_final':[g['cash']for g in games],'rival_final':[g['opponent_cash']for g in games],'margins':[g['cash']-g['opponent_cash']for g in games],'shops':games[0]['shops']})
        with (RUN/f'{family}_inputs.txt').open('w')as f:
            f.write(str(len(models))+'\n');f.write(' '.join(flatten([[m['sales'],m['buys'],m['fixed_costs']]for m in models]))+'\n');f.write(str(len(inputs))+'\n')
            for row in inputs:f.write(' '.join(flatten(row))+'\n')
        (RUN/f'{family}_expected.json').write_text(json.dumps(expected,separators=(',',':'))+'\n');lineage[family]={'source_sha256':hashes,'contexts':len(inputs),'branches':cfg['branches'],'scope':'Only fixed own flow plans plus replay scenario IDs/shops are reconstruction inputs. Driver reveals shops to AgentObservation only on their actual reveal dates. Future actual outcomes are scoring data only.'}
    (RUN/'BRANCH_INPUT_LINEAGE.json').write_text(json.dumps(lineage,indent=2)+'\n');print('Prepared Atakan and sheep common320context panels.')


if __name__=='__main__':main()
