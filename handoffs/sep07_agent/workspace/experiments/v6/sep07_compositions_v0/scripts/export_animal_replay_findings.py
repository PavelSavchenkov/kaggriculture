"""Package replay evidence and fixed courses as offline data, never gameplay."""
import csv
import hashlib
import itertools
import json
import shutil
from collections import Counter, defaultdict
from functools import lru_cache
from pathlib import Path

from analyze_animal_decision_replays import ANIMALS, COHORTS, EXP, OUT, canonical, digest, rows, write_csv
from generate_public_routes import ITEMS, MARKET, OPS, triple

BRANCHES = {'cow': (106371597, 1), 'sheep': (106380079, 1), 'goose': (106374592, 1)}
PAIR_SPECS = [
    ('atakan_cow_sheep', 106371597, 1, 106380079, 1, 226),
    ('atakan_cow_goose', 106371597, 1, 106374592, 1, 226),
    ('jeonghun_cow_goose', 106385432, 1, 106398132, 0, 150),
    ('jeonghun_sheep_goose', 106390091, 1, 106398132, 0, 150),
    ('mengfei_sheep_cow', 106373411, 0, 106381779, 0, 169),
    ('justin_late_species', 106370340, 0, 106415893, 1, 196),
]


def dump(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n')


@lru_cache(maxsize=2)
def replay(episode):
    return json.loads((EXP / f'replays/episode-{episode}-replay.json').read_text())


def ordered(value):
    if isinstance(value, dict):
        return [[k, ordered(v)] for k, v in value.items()]
    if isinstance(value, list):
        return [ordered(v) for v in value]
    return value


def state_at(episode, seat, step):
    r = replay(episode)
    public = r['steps'][step][0]['observation']
    own = public['farms'][seat]
    private = r['steps'][step][seat]['observation']['private']
    physical = {'farm': {k: v for k, v in own.items() if k != 'money'}, 'private_ordered': ordered(private)}
    return {'own_farm': own, 'own_private': private, 'own_private_ordered': ordered(private),
            'own_physical_sha256_excluding_money': digest(physical),
            'own_farm_sha256_excluding_money': digest(physical['farm']),
            'public_market': public['market'], 'town': public['town'],
            'opponent_farm': public['farms'][seat ^ 1], 'day': public['day'], 'hour': public['hour']}


def difference(a, b, prefix=''):
    if type(a) is not type(b):
        return [{'path': prefix, 'a': a, 'b': b}]
    if isinstance(a, dict):
        result = []
        for key in sorted(a.keys() | b.keys()):
            path = f'{prefix}/{key}'
            if key not in a or key not in b:
                result.append({'path': path, 'a': a.get(key), 'b': b.get(key)})
            else:
                result.extend(difference(a[key], b[key], path))
        return result
    if isinstance(a, list):
        if len(a) != len(b):
            return [{'path': prefix, 'a': a, 'b': b}]
        return [d for i, (x, y) in enumerate(zip(a, b)) for d in difference(x, y, f'{prefix}/{i}')]
    return [] if a == b else [{'path': prefix, 'a': a, 'b': b}]


def actions(episode, seat):
    return [s[seat]['action'] for s in replay(episode)['steps'][1:720]]


def main():
    games = json.loads((OUT / 'games.json').read_text())
    purchases = json.loads((OUT / 'purchases.json').read_text())
    placements = json.loads((OUT / 'placements.json').read_text())
    gm = {(x['episode'], x['seat']): x for x in games}
    program_metadata = json.loads((EXP / 'league/top_replay_library/IMPORT.json').read_text())['programs']
    programs = {(x['episode'], x['seat']): x for x in program_metadata}
    evidence = OUT / 'exact_cases'
    evidence.mkdir(exist_ok=True)
    comparisons = []
    for name, ea, sa, eb, sb, step in PAIR_SPECS:
        a, b = state_at(ea, sa, step), state_at(eb, sb, step)
        aa, bb = actions(ea, sa), actions(eb, sb)
        shared = next((t for t, (x, y) in enumerate(zip(aa, bb)) if x != y), 719)
        record = {'id': name, 'step': step, 'day': step // 24, 'hour': step % 24,
                  'source_a': gm[ea, sa], 'source_b': gm[eb, sb],
                  'same_submission': gm[ea, sa]['submission_id'] == gm[eb, sb]['submission_id'],
                  'identical_raw_action_prefix_length': shared,
                  'own_farm_differences': difference(a['own_farm'], b['own_farm']),
                  'own_private_differences': difference(a['own_private'], b['own_private']),
                  'own_private_ordered_equal': a['own_private_ordered'] == b['own_private_ordered'],
                  'own_physical_equal_except_money': a['own_physical_sha256_excluding_money'] == b['own_physical_sha256_excluding_money'],
                  'state_a': a, 'state_b': b, 'action_a': aa[step], 'action_b': bb[step]}
        dump(evidence / f'{name}.json', record)
        comparisons.append({k: v for k, v in record.items() if k not in ('source_a', 'source_b', 'state_a', 'state_b')})
    dump(OUT / 'state_comparisons.json', comparisons)

    portfolio = OUT / 'atakan_three_way'
    portfolio.mkdir(exist_ok=True)
    courses, manifest = {}, []
    for label, (episode, seat) in BRANCHES.items():
        directory = portfolio / label
        directory.mkdir(exist_ok=True)
        game = gm[episode, seat]
        r = replay(episode)
        raw = actions(episode, seat)
        typed, normalized = [], []
        for step, action in enumerate(raw):
            units = [triple(x) for x in [action['farmer'], *action['hands']]]
            orders = [triple(x, True) for x in action['market']]
            typed.append({'units': units, 'market': orders})
            n = len(r['steps'][step][0]['observation']['farms'][seat]['hands']) + 1
            normalized.append({'units': (units + [[0, 0, 0]] * n)[:n], 'market': orders})
        assert len(raw) == len(normalized) == 719
        courses[label] = normalized
        dump(directory / 'raw_actions_719.json', raw)
        dump(directory / 'typed_tape_719.json', typed)
        dump(directory / 'normalized_actions_719.json', normalized)
        dump(directory / 'state_before_226.json', state_at(episode, seat, 226))
        dump(directory / 'animal_purchases.json', [x for x in purchases if (x['episode'], x['seat']) == (episode, seat)])
        dump(directory / 'animal_placements.json', [x for x in placements if (x['episode'], x['seat']) == (episode, seat)])
        source_dir = EXP / 'research' / game['cohorts'][0]
        copied = {}
        extracts = {}
        for filename in ('animal_instances.csv', 'crop_instances.csv', 'animal_days.csv', 'crop_days.csv',
                         'unit_events.csv', 'transactions.csv', 'daily_product.csv', 'tile_transitions.csv'):
            selected = [x for x in rows(source_dir / filename) if int(x['episode_id']) == episode and x['team'] == game['team']]
            write_csv(directory / filename, selected)
            copied[str((source_dir / filename).relative_to(EXP))] = hashlib.sha256((source_dir / filename).read_bytes()).hexdigest()
            extracts[filename] = selected
        composition = json.loads((source_dir / 'compositions.json').read_text())
        proposal = next(x for x in composition['proposals'] if x['episode'] == episode and x['seat'] == seat)
        dump(directory / 'composition.json', {'items': composition['items'], 'day_convention': composition['day_convention'], 'proposal': proposal})
        template_path = source_dir / f'day_templates/episode_{episode}_seat_{seat}.json.gz'
        shutil.copyfile(template_path, directory / 'day_templates.json.gz')
        copied[str(template_path.relative_to(EXP))] = hashlib.sha256(template_path.read_bytes()).hexdigest()
        daily = []
        for day in range(30):
            events = [x for x in extracts['unit_events.csv'] if int(x['day']) == day]
            trades = [x for x in extracts['transactions.csv'] if int(x['day']) == day]
            harvest = Counter()
            service = Counter()
            for event in events:
                if event['operation'] == 'HARVEST':
                    source = event['source']
                    item = {'COW': 'MILK', 'SHEEP': 'WOOL', 'GOOSE': 'EGG'}.get(source, source)
                    harvest[item] += int(event['quantity'])
                if event['operation'] in ('FEED', 'CARE', 'COLLECT_FERTILIZER', 'WATER', 'FERTILIZE'):
                    service[f"{event['operation']}:{event['source']}"] += 1
            daily.append({'day': day, 'recorded_successful_service_counts': dict(service),
                          'realized_harvest_units': dict(harvest), 'trades': trades})
        dump(directory / 'daily_service_harvest_trade.json', daily)
        assert programs[episode, seat]['replay_sha256'] == game['replay_sha256']
        entry = {'branch_label': label, 'source': game, 'existing_library_program': programs[episode, seat]['program'],
                 'raw_actions_sha256': digest(raw), 'typed_tape_sha256': digest(typed),
                 'normalized_actions_sha256': digest(normalized), 'input_sha256': copied,
                 'artifacts_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(directory.iterdir())}}
        dump(directory / 'IMPORT.json', entry)
        manifest.append(entry)
    prefix = {}
    for a, b in itertools.combinations(courses, 2):
        prefix[f'{a}:{b}'] = next((t for t, (x, y) in enumerate(zip(courses[a], courses[b])) if x != y), 719)
    assert all(x == 226 for x in prefix.values()), prefix
    dump(portfolio / 'MANIFEST.json', {'branches': manifest, 'normalized_pair_prefix_lengths': prefix,
        'branch_step': 226, 'day': 9, 'hour': 10, 'encoding': {'unit_ops': OPS, 'market_ops': MARKET, 'items': ITEMS},
        'normalization': ['The same generate_public_routes.triple encoder as the existing verified C++ replay library',
                          'typed_tape retains all source worker commands for runtime active-worker normalization',
                          'normalized_actions pads PASS and truncates to active source-observation workers',
                          'Engine-ignored fourth and later arguments and PASS/HIRE/BUY_LAND arguments are omitted'],
        'production_scope': 'Exact dated life intervals, observed daily service, realized harvest, trades, and day-start/end states; harvest is not gross production, which can be capped or lost before collection',
        'branch_inference_scope': 'Fixed observed complete courses. Neither private branch formulas nor counterfactual strength is established.'})

    selected = [x for x in purchases if
                (x['team'] == 'Atakan Aldemir' and x['step'] in (226, 241)) or
                (x['team'] == '3정훈' and x['step'] in (150, 169, 176)) or
                (x['team'] == 'Mengfei Li' and x['step'] in (169, 176)) or
                (x['team'] == 'JustinLee' and x['step'] in (150, 173, 178, 217, 226, 241)) or
                (x['team'] == 'ymg_aq' and x['step'] in (73, 145, 361, 362, 433, 505)) or
                (x['team'] == '自己找差距' and x['step'] == 361)]
    dump(OUT / 'key_purchase_cases.json', selected)
    write_csv(OUT / 'key_purchase_cases.csv', selected)
    reuse = [x for x in placements if any(z['value'] in ('COOP', 'PASTURE') for z in x['prior_tile_occupants'][:-1])]
    dump(OUT / 'structure_reuse_cases.json', reuse)
    nulls = [x for x in purchases if x['actual'] < x['requested'] or x['requested'] <= 0]
    dump(OUT / 'failed_and_zero_orders.json', nulls)
    stats = {'unique_player_games': len(games), 'distinct_players': len({x['team'] for x in games}),
             'purchase_order_records': len(purchases), 'successful_animal_units_bought': sum(x['actual'] for x in purchases),
             'successful_placements': len(placements), 'failed_positive_orders': sum(x['actual'] < x['requested'] for x in purchases),
             'zero_orders': sum(x['requested'] == 0 for x in purchases),
             'placements_with_closer_ready_structure': sum(x['closer_ready_structures'] > 0 for x in placements),
             'placements_with_closer_buildable_site': sum(x['closer_buildable_sites'] > 0 for x in placements),
             'placements_on_former_crop': sum(any(z['value'] in ITEMS[:5] for z in x['prior_tile_occupants']) for x in placements),
             'structure_reuse_cases': len(reuse),
             'positive_purchases_one_turn_after_reveal': sum(x['actual'] > 0 and x['turns_since_last_shop'] == 1 for x in purchases),
             'positive_purchases_after_step_360': sum(x['actual'] > 0 and x['step'] >= 360 for x in purchases)}
    dump(OUT / 'statistics.json', stats)
    print(json.dumps({'statistics': stats, 'atakan_prefixes': prefix, 'programs': {x['branch_label']: x['existing_library_program'] for x in manifest}}, ensure_ascii=False))


if __name__ == '__main__':
    main()
