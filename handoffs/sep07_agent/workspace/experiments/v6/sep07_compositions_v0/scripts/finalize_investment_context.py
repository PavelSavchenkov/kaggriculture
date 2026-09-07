"""Summarize exact paired promotion evidence and its causal scope."""
import hashlib
import json
import math
from pathlib import Path
from statistics import mean

EXP = Path(__file__).resolve().parents[1]
NEW = 'investment_context_guarded_001_best'
OLD = 'animal_adaptive_r1_c0_b0'
FRESH = EXP / 'results/investment_context_fresh_1300000'
CHECKS = EXP / 'results/investment_context_checks'


def read(path):
    return json.loads(path.read_text())


def summarize(path):
    data = read(path)
    games = data.pop('games')
    margins = sorted(g['cash'] - g['opponent_cash'] for g in games)
    cash = sorted(g['cash'] for g in games)
    count = len(games)
    tail = math.ceil(count / 10)
    wins = sum(m > 0 for m in margins)
    ties = margins.count(0)
    data.update(games=count, wins=wins, ties=ties, losses=count-wins-ties,
                win_utility=(wins + .5*ties)/count, mean_margin=mean(margins), mean_cash=mean(cash),
                margin_cvar10=mean(margins[:tail]), cash_cvar10=mean(cash[:tail]),
                pass_J=.8*mean(cash)+.2*mean(cash[:tail]), path=str(path.relative_to(EXP)))
    return data


def main():
    old_report = read(EXP / 'results/animal_investment_validation.json')
    groups = old_report['grouping']
    report = {'candidate': NEW, 'parent': OLD,
              'status': 'Promoted after complete exact operational and paired league checks',
              'scenario': 'Official rules; independent-shop promotion and native-RNG audit',
              'seed_scope': 'Fresh 1300000..1300511, both seats; native 14000..14127, both seats',
              'objective': 'Equal opponent-group mean win utility, with margin and lower tail reported separately',
              'grouping': groups, 'fresh': {}, 'paired': {}}
    for agent in [NEW, OLD]:
        results = {p.stem.split('_vs_', 1)[1]: summarize(p) for p in FRESH.glob(f'{agent}_vs_*.json')}
        group_scores = {group: mean(results[b]['win_utility'] for b in rivals) for group, rivals in groups.items()}
        extended = {k: list(v) for k,v in groups.items()}
        extended['prior_versions'].append(OLD)
        extended['public_controllers'].append('public_router_v5')
        report['fresh'][agent] = {'opponents': results, 'group_win_utility': group_scores,
                                 'equal_group_win_utility': mean(group_scores.values()),
                                 'equal_opponent_win_utility': mean(r['win_utility'] for r in results.values()),
                                 'expanded_group_win_utility': mean(mean(results[b]['win_utility'] for b in rivals) for rivals in extended.values())}
    for rival in report['fresh'][NEW]['opponents']:
        new = read(FRESH / f'{NEW}_vs_{rival}.json')['games']
        old = read(FRESH / f'{OLD}_vs_{rival}.json')['games']
        assert [(g['seed'],g['seat']) for g in new] == [(g['seed'],g['seat']) for g in old]
        delta = [(a['cash']-a['opponent_cash'])-(b['cash']-b['opponent_cash']) for a,b in zip(new,old)]
        report['paired'][rival] = {'margin_gain': mean(delta), 'games_improved': sum(d>0 for d in delta),
                                  'games_equal': delta.count(0), 'games_worse': sum(d<0 for d in delta),
                                  'wins_gain': report['fresh'][NEW]['opponents'][rival]['wins']-report['fresh'][OLD]['opponents'][rival]['wins']}
        assert report['paired'][rival]['margin_gain'] >= 0
        assert report['fresh'][NEW]['opponents'][rival]['win_utility'] >= report['fresh'][OLD]['opponents'][rival]['win_utility']
    a = read(CHECKS / f'{NEW}_profile64.json')['games']
    b = read(CHECKS / f'{OLD}_profile64.json')['games']
    causal = {'games':len(a)}
    fields = ['produced','sold','discarded','opponent_cash','opponent_action_hash']
    causal['unchanged'] = {k:sum(x[k]==y[k] for x,y in zip(a,b)) for k in fields}
    causal['unchanged'].update({k:sum(x['profile'][k]==y['profile'][k] for x,y in zip(a,b))
                                for k in ['buys','seed_buys','buy_hours','sell_hours']})
    life_key = lambda g: sorted(tuple(l[:3]+l[5:]) for l in g['profile']['lives'])
    causal['unchanged']['biology_and_service'] = sum(life_key(x)==life_key(y) for x,y in zip(a,b))
    causal['mean_delta'] = {k:mean(x[k]-y[k] for x,y in zip(a,b)) for k in ['cash','opponent_cash','unit_faults']}
    causal['mean_delta'].update({k:mean(x['profile'][k]-y['profile'][k] for x,y in zip(a,b)) for k in ['hires','hire_cost']})
    changed = [(x,y) for x,y in zip(a,b) if x['action_hash']!=y['action_hash']]
    causal['changed_games'] = len(changed)
    causal['changed_delta_ranges'] = {k:sorted({x['profile'][k]-y['profile'][k] for x,y in changed}) for k in ['hires','hire_cost']}
    causal['changed_fault_delta'] = sorted({x['unit_faults']-y['unit_faults'] for x,y in changed})
    report['causal'] = causal
    report['checks'] = read(CHECKS / 'CHECKS.json')
    report['native_and_pass'] = {p.stem:summarize(p) for p in CHECKS.glob('*.json') if '_native_' in p.stem or '_pass' in p.stem}
    report['lineage'] = read(EXP / f'candidates/{NEW}/IMPORT.json')
    paths = ['include/guarded_day.hpp', 'include/deferred_animal.hpp', 'include/animal_investment_value.hpp',
             f'candidates/{NEW}/source/agent.hpp', 'runs/investment_day_contexts_001/days.hpp',
             'build/8dee9d8af503a3ed5b93/arena', 'build/90ed0c310648e0b12674/arena']
    report['artifact_sha256'] = {p:hashlib.sha256((EXP/p).read_bytes()).hexdigest() for p in paths}
    report['limitations'] = ['General animal choice currently governs one added opportunity; earlier two-animal shop choice remains milk/wool.',
                             'Waiting is operational but no winning waiting setting established.',
                             'New day plans cover late sheep after either six or eight earlier cows; other herd cases use inherited execution.',
                             'Physical day guards are not future finance guarantees.',
                             'This is a local improvement after the single authorized official submission; no further upload made.']
    destination = EXP / 'results/investment_context_validation.json'
    destination.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'objective':{a:report['fresh'][a]['equal_group_win_utility'] for a in [NEW,OLD]},
                      'paired': report['paired'], 'causal':causal}, indent=2))


if __name__ == '__main__':
    main()
