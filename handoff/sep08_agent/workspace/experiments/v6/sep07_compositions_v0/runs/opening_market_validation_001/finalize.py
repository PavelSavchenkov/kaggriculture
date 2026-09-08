from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import math
import numpy as np

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
CHECKS=EXP/'runs/opening_market_checks_001'


def read(path):return json.loads(path.read_text())


def metrics(games):
    margin=np.array([g['cash']-g['opponent_cash'] for g in games])
    u=(margin>0)+0.5*(margin==0)
    cash=np.array([g['cash'] for g in games])
    return {'games':len(games),'wins':int((margin>0).sum()),'ties':int((margin==0).sum()),
            'losses':int((margin<0).sum()),'utility':float(u.mean()),'mean_margin':float(margin.mean()),
            'margin_cvar10':float(np.sort(margin)[:math.ceil(len(games)/10)].mean()),
            'mean_cash':float(cash.mean()),
            'pass_J':float(.8*cash.mean()+.2*np.sort(cash)[:math.ceil(len(games)/10)].mean())},u


def contrast(a,b,profiles=False):
    assert [(g['seed'],g['seat']) for g in a]==[(g['seed'],g['seat']) for g in b]
    ma,_=metrics(a);mb,_=metrics(b)
    row={k+'_gain':ma[k]-mb[k] for k in ['utility','mean_margin','margin_cvar10','mean_cash']}
    for k in ['opponent_cash','unit_faults','worker_days']:
        row[k+'_gain']=float(np.mean([x[k]-y[k] for x,y in zip(a,b)]))
    for k in ['produced','sold','discarded']:
        row[k+'_gain']=np.mean([np.array(x[k])-y[k] for x,y in zip(a,b)],axis=0).tolist()
    row['output_identical_games']=sum(x['produced']==y['produced'] for x,y in zip(a,b))
    row['rival_actions_changed']=sum(x['opponent_action_hash']!=y['opponent_action_hash'] for x,y in zip(a,b))
    if profiles:
        for k in ['hires','hire_cost','land','land_cost','weed_digs']:
            row[k+'_gain']=float(np.mean([x['profile'][k]-y['profile'][k] for x,y in zip(a,b)]))
        row['buys_gain']=np.mean([np.array(x['profile']['buys'])-y['profile']['buys'] for x,y in zip(a,b)],axis=0).tolist()
    return row


def main():
    spec=read(RUN/'PREREGISTERED.json');new,old,control=[spec[k] for k in ['candidate','baseline','diagnostic_control']]
    records={};utilities={};games={};hashes={}
    for agent in [new,old,control]:
        records[agent]={}
        for opponent in spec['opponents']:
            p=RUN/f'fresh/{agent}_vs_{opponent}.json';g=read(p)['games']
            assert len(g)==1024 and all(x['turns']==719 for x in g)
            assert [(x['seed'],x['seat']) for x in g]==[(seed,seat) for seed in range(1760000,1760512) for seat in range(2)]
            games[agent,opponent]=g;records[agent][opponent],utilities[agent,opponent]=metrics(g)
            hashes[str(p.relative_to(EXP))]=hashlib.sha256(p.read_bytes()).hexdigest()
    paired={o:contrast(games[new,o],games[old,o]) for o in spec['opponents']}
    control_paired={o:contrast(games[new,o],games[control,o]) for o in spec['opponents']}
    draws=np.random.default_rng(17604121).integers(0,512,(10000,512))
    grouped={}
    for label,key in [('current','grouping'),('historical','historical_grouping')]:
        values={a:np.mean([np.mean([utilities[a,o] for o in opps],axis=0) for opps in spec[key].values()],axis=0) for a in [new,old,control]}
        gain=(values[new]-values[old]).reshape(-1,2).mean(axis=1)
        grouped[label]={'utilities':{a:float(x.mean()) for a,x in values.items()},'gain_95pct':np.quantile(gain[draws].mean(axis=1),[.025,.975]).tolist()}
    checks=read(CHECKS/'CHECKS.json');assert checks['generic_pair_debug_thread_all_game_records_equal']
    assert read(RUN/'WRAPPER_PARITY.json')['full_records_exact']==5120
    frozen=read(RUN/'frozen/FROZEN.json')
    native={}
    for o in [old,'crop_mix_t2_wheat','investment_context_guarded_001_best','teammate_shoprouter','king_rc4','public_router','public_router_v5','public_sixday']:
        ng={a:read(CHECKS/f'{a}_native_{o}.json')['games'] for a in [new,old]}
        native[o]={'metrics':{a:metrics(g)[0] for a,g in ng.items()},'paired':contrast(ng[new],ng[old])}
    passes={a:metrics(read(CHECKS/f'{a}_pass256.json')['games'])[0] for a in [new,old]}
    profiles={}
    for o in [old,'teammate_shoprouter','king_rc4','public_router_v5','public_sixday']:
        p={a:read(RUN/f'profiles/{a}_vs_{o}.json')['games'] for a in [new,old,control]}
        profiles[o]={'vs_parent':contrast(p[new],p[old],True),'quantity_only_vs_control':contrast(p[new],p[control],True)}
    gates={
        'current_group_positive_95pct':grouped['current']['gain_95pct'][0]>0,
        'historical_group_noninferiority':grouped['historical']['gain_95pct'][0]>-.0025,
        'direct_parent_positive':records[new][old]['utility']>.5 and records[new][old]['mean_margin']>0,
        'all_paired_mean_margins_nonnegative':min(p['mean_margin_gain'] for p in paired.values())>=0,
        'individual_utility_regression_limit':min(p['utility_gain'] for p in paired.values())>=-.02,
        'native_direct_positive':native[old]['metrics'][new]['utility']>.5 and native[old]['metrics'][new]['mean_margin']>0,
        'native_teammate_paired_margin_positive':native['teammate_shoprouter']['paired']['mean_margin_gain']>0,
        'operational_and_wrapper_parity':True,
    }
    tails={o:p['margin_cvar10_gain'] for o,p in paired.items() if p['margin_cvar10_gain']<0}
    report={'created_utc':datetime.now(timezone.utc).isoformat(),'candidate':new,'parent':old,
            'status':'Numeric gates passed; final tail/causal review required' if all(gates.values()) else 'Not promoted: gate failed',
            'preregistered':spec,'gates':gates,'grouped':grouped,'fresh':records,'paired':paired,
            'vs_diagnostic_control':control_paired,'native':native,'pass':passes,'causal_profiles':profiles,
            'fresh_tail_regressions':tails,'wrapper_parity_records':5120,'frozen_dependency_count':len(frozen['files_sha256']),
            'files_sha256':hashes,'limitations':['Most old-opponent gains are about$5; large gain is against the newly introduced Bohann parent.',
              'Two fresh opponents lose one strict win each despite positive mean margins. All tails and native tradeoffs are reported.',
              'Current and historical league groupings differ explicitly; no large historical utility gain is claimed.',
              'Opening market interactions can change opponent liquidity and later behavior. New crop/animal construction remains separate work.',
              'No new official catalog update, commit or Kaggle upload is authorized for this candidate.']}
    (EXP/'results/opening_market_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    (RUN/'profiles/CAUSAL.json').write_text(json.dumps(profiles,indent=2)+'\n')
    print(report['status']);print('gates',gates);print('grouped',grouped);print('direct',records[new][old]);print('tail regressions',tails)
    print('native',{o:{'wins':v['metrics'][new]['wins'],'margin_gain':v['paired']['mean_margin_gain'],'utility_gain':v['paired']['utility_gain'],'tail_gain':v['paired']['margin_cvar10_gain']} for o,v in native.items()})


if __name__=='__main__':main()
