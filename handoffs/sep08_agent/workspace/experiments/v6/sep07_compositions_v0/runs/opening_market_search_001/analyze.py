from pathlib import Path
import csv
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
BASE = 'bohann_opening_v1'
OLD = 'crop_mix_t2_wheat'
HISTORICAL = {
    'prior_versions': ['shop_herd_guarded_001_best','shop_herd_s6_m3_g1','opening_router_v4'],
    'teammate': ['teammate_shoprouter'],
    'public_controllers': ['king_rc4','public_router','public_capacity_router','public_sixday'],
    'top_replays': ['binghua_116','junghoon_wool_sales','john_131','ticket_p116_t9_i9'],
}
CURRENT = {
    'prior_versions': ['shop_herd_guarded_001_best','shop_herd_s6_m3_g1','opening_router_v4',
                       'animal_adaptive_r1_c0_b0','investment_context_guarded_001_best',
                       'crop_value_m2_t4','crop_rotation_t2_berry','wheat_one_fert',OLD,BASE],
    'teammate': ['teammate_shoprouter'],
    'public_controllers': ['king_rc4','public_router','public_capacity_router','public_sixday','public_router_v5'],
    'top_replays': HISTORICAL['top_replays'],
}


def utility(game):
    return 1.0 if game['cash'] > game['opponent_cash'] else 0.5 if game['cash'] == game['opponent_cash'] else 0.0


def measure(games):
    margin = sorted(g['cash']-g['opponent_cash'] for g in games)
    return {'games':len(games), 'wins':sum(utility(g)==1 for g in games),
            'ties':sum(utility(g)==0.5 for g in games), 'utility':statistics.mean(map(utility,games)),
            'mean_margin':statistics.mean(margin), 'margin_cvar10':statistics.mean(margin[:(len(games)+9)//10]),
            'mean_cash':statistics.mean(g['cash'] for g in games),
            'mean_rival_cash':statistics.mean(g['opponent_cash'] for g in games)}


def grouped(values, groups, key):
    return statistics.mean(statistics.mean(values[o][key] for o in names) for names in groups.values())


def main():
    files = sorted((RUN/'discovery').glob('*.json'))+sorted((RUN/'extended_discovery').glob('*.json'))
    records = {}
    source_hashes = {}
    for path in files:
        data = json.loads(path.read_text())
        key=(data['agent_a'],data['agent_b'])
        assert key not in records,key
        records[key]=data['games']
        assert len(data['games'])==128 and all(g['turns']==719 for g in data['games'])
        source_hashes[str(path.relative_to(RUN))]=hashlib.sha256(path.read_bytes()).hexdigest()
    opponents=sorted(o for a,o in records if a==BASE)
    assert set(opponents)=={o for names in CURRENT.values() for o in names}
    choices=list(csv.DictReader((RUN/'discovery/choices.csv').open()))
    controls={}
    for control,baseline in [('q81_b13_m0',BASE),('q0_b30_m2',OLD)]:
        for o in opponents:assert records[control,o]==records[baseline,o],(control,o)
        controls[control]=128*len(opponents)
    base={o:measure(records[BASE,o]) for o in opponents}
    phenotypes={}
    results=[]
    for choice in choices:
        name=choice['name']; per={}; hash_=hashlib.sha256()
        for o in opponents:
            games=records[name,o];old=records[BASE,o]; value=measure(games)
            assert [(g['seed'],g['seat']) for g in games]==[(g['seed'],g['seat']) for g in old]
            for key in ['utility','mean_margin','margin_cvar10','mean_cash','mean_rival_cash']:
                value[key+'_gain']=value[key]-base[o][key]
            value['own_output_same_games']=sum(a['produced']==b['produced'] for a,b in zip(games,old))
            value['opponent_actions_changed']=sum(a['opponent_action_hash']!=b['opponent_action_hash'] for a,b in zip(games,old))
            per[o]=value
            hash_.update(json.dumps(games,sort_keys=True,separators=(',',':')).encode())
        signature=hash_.hexdigest()
        row={'choice':choice,'per_opponent':per,'full_record_sha256':signature,'duplicate_of':phenotypes.get(signature),
             'pooled_margin_gain':statistics.mean(v['mean_margin_gain'] for v in per.values())}
        for label,groups in [('historical',HISTORICAL),('current',CURRENT)]:
            row[label+'_group_utility']=grouped(per,groups,'utility')
            row[label+'_group_utility_gain']=grouped(per,groups,'utility_gain')
            row[label+'_group_margin_gain']=grouped(per,groups,'mean_margin_gain')
        phenotypes.setdefault(signature,name)
        results.append(row)
    results.sort(key=lambda r:(r['current_group_utility_gain'],r['pooled_margin_gain']),reverse=True)
    report={'scope':'Discovery only; 64seeds both seats,20opponents. Current grouping explicitly adds V5and recent local parents; historical grouping is retained for comparison. No fresh promotion evidence.',
            'groups':{'historical':HISTORICAL,'current':CURRENT},'baseline':base,
            'controls_exact_records':controls,'choices':len(choices),'unique_full_record_phenotypes':len(phenotypes),
            'total_games':sum(len(g) for g in records.values()),'results':results,'source_sha256':source_hashes}
    (RUN/'EXTENDED_SCREEN.json').write_text(json.dumps(report,indent=2)+'\n')
    for row in results:
        per=row['per_opponent']
        print(row['choice']['name'], 'group_gain',round(row['current_group_utility_gain'],6),
              'margin_gain',round(row['pooled_margin_gain'],2),
              'direct',per[BASE]['wins'],per[BASE]['ties'],round(per[BASE]['mean_margin'],2),
              'worst',min((round(v['utility_gain'],5),o) for o,v in per.items()))


if __name__=='__main__':
    main()
