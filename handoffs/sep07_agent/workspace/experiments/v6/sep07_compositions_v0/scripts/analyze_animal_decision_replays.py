"""Extract observed decisions from fixed public replays; no agent gameplay."""
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
OUT = EXP / 'research/animal_decision_replays'
COHORTS = ['refresh_0804', 'refresh_0914', 'refresh_1010']
ANIMALS = ('GOOSE', 'COW', 'SHEEP')
PRODUCTS = {'GOOSE': 'EGG', 'COW': 'MILK', 'SHEEP': 'WOOL'}
COSTS = {'GOOSE': 300, 'COW': 400, 'SHEEP': 500}
SHOP_PRODUCTS = {'BAKERY': ('EGG', 'WHEAT'), 'PIZZA_SHOP': ('MILK', 'TOMATO', 'WHEAT'),
                 'BRUNCH_SPOT': ('EGG', 'WHEAT', 'STRAWBERRY'), 'YARN_STORE': ('WOOL',),
                 'ICE_CREAM_SHOP': ('STRAWBERRY', 'MILK', 'WHEAT'), 'PET_CAFE': ('CARROT',),
                 'SMOOTHIE_SHOP': ('STRAWBERRY', 'MILK'), 'FARMERS_MARKET': ('WHEAT', 'CARROT', 'TOMATO', 'STRAWBERRY')}


def rows(path):
    return list(csv.DictReader(path.open()))


def canonical(value):
    return json.dumps(value, separators=(',', ':'), sort_keys=True)


def digest(value):
    return hashlib.sha256(canonical(value).encode()).hexdigest()


def counts(farm):
    return Counter(t['animal'] for row in farm['tiles'] for t in row if isinstance(t, dict) and t.get('animal'))


def empty_structures(farm):
    return {kind: [(x, y) for y, row in enumerate(farm['tiles']) for x, t in enumerate(row)
                   if isinstance(t, dict) and t.get('kind') == kind and not t.get('animal')]
            for kind in ('COOP', 'PASTURE')}


def demand(shops):
    result = Counter()
    for shop in shops:
        products = SHOP_PRODUCTS[shop]
        for item in products:
            result[item] += 2 if len(products) == 1 else 1
    return {PRODUCTS[a]: result[PRODUCTS[a]] for a in ANIMALS}


def write_csv(path, records):
    keys = list(dict.fromkeys(k for r in records for k in r))
    with path.open('w') as target:
        writer = csv.DictWriter(target, keys)
        writer.writeheader()
        for r in records:
            writer.writerow({k: canonical(v) if isinstance(v, (dict, list, tuple)) else v for k, v in r.items()})


def main():
    OUT.mkdir(exist_ok=True)
    games = {}
    transactions = defaultdict(list)
    units = defaultdict(list)
    source_hashes = {}
    for cohort in COHORTS:
        directory = EXP / 'research' / cohort
        for filename in ('top_replay_manifest.csv', 'transactions.csv', 'unit_events.csv', 'animal_instances.csv'):
            source_hashes[str((directory / filename).relative_to(EXP))] = hashlib.sha256((directory / filename).read_bytes()).hexdigest()
        manifest = rows(directory / 'top_replay_manifest.csv')
        new = set()
        for row in manifest:
            key = int(row['episode_id']), row['team']
            if key not in games:
                games[key] = dict(row, cohorts=[cohort])
                new.add(key)
            else:
                games[key]['cohorts'].append(cohort)
        for row in rows(directory / 'transactions.csv'):
            key = int(row['episode_id']), row['team']
            if key in new:
                transactions[key].append(row)
        for row in rows(directory / 'unit_events.csv'):
            key = int(row['episode_id']), row['team']
            if key in new:
                units[key].append(row)
    purchases, placements, reveals, summaries = [], [], [], []
    action_streams, event_streams = {}, {}
    by_episode = defaultdict(list)
    for key in games:
        by_episode[key[0]].append(key)
    for episode, keys in sorted(by_episode.items()):
        path = EXP / 'replays' / f'episode-{episode}-replay.json'
        replay_bytes = path.read_bytes()
        replay_hash = hashlib.sha256(replay_bytes).hexdigest()
        replay = json.loads(replay_bytes)
        for key in keys:
            meta = games[key]
            seat = replay['info']['TeamNames'].index(key[1])
            assert replay['info']['TeamNames'].count(key[1]) == 1
            context = {'episode': episode, 'seat': seat, 'team': key[1], 'team_id': int(meta['team_id']),
                       'submission_id': int(meta['submission_id']), 'rank': int(meta['rank']),
                       'leaderboard_score': float(meta['leaderboard_score']), 'episode_create_time': meta['episode_create_time'],
                       'replay_sha256': replay_hash, 'replay_file': str(path.relative_to(EXP)), 'cohorts': meta['cohorts']}
            actions = [pair[seat]['action'] for pair in replay['steps'][1:720]]
            action_streams[key] = actions
            trades = defaultdict(list)
            for row in transactions[key]:
                trades[24 * int(row['day']) + int(row['hour'])].append(row)
            unit_events = defaultdict(list)
            for row in units[key]:
                unit_events[int(row['result_index']) - 1].append(row)
            prefix = hashlib.sha256()
            last_reveal = None
            known_shops = []
            local_purchases, local_placements = [], []
            for step, action in enumerate(actions):
                public = replay['steps'][step][0]['observation']
                private = replay['steps'][step][seat]['observation']['private']
                own = public['farms'][seat]
                shops = public['town']['unlocked_shops']
                dt = demand(shops)
                common = dict(context, step=step, day=public['day'], hour=public['hour'], shops=list(shops),
                              demand_per_cycle=dt, own_money=own['money'], opponent_money=public['farms'][seat ^ 1]['money'],
                              prices={p: public['market']['prices'][p] for p in PRODUCTS.values()},
                              herd=dict(counts(own)), empty_structures=empty_structures(own),
                              held_animals={a: private['shed'].get(a, 0) + sum(inv.get(a, 0) for inv in private['inventories']) for a in ANIMALS},
                              prefix_actions_sha256=prefix.hexdigest(), action_sha256=digest(action))
                if len(shops) > len(known_shops):
                    assert shops[:len(known_shops)] == known_shops
                    last_reveal = step
                    known_shops = list(shops)
                    reveals.append(dict(common, new_shop=shops[-1]))
                common['turns_since_last_shop'] = None if last_reveal is None else step - last_reveal
                market = action.get('market', [])[:10]
                trade_cursor = 0
                money = float(own['money'])
                for order_index, order in enumerate(market):
                    if not order:
                        continue
                    operation = order[0]
                    parsed = operation in ('HIRE', 'BUY_LAND') or (operation in ('BUY_SEED', 'BUY_PRODUCT', 'BUY_ANIMAL', 'SELL') and len(order) >= 3 and int(order[2]) > 0)
                    actual, value = 0, 0.0
                    if parsed:
                        trade = trades[step][trade_cursor]
                        trade_cursor += 1
                        assert trade['operation'] == operation, (key, step, order, trade)
                        if operation not in ('HIRE', 'BUY_LAND'):
                            assert order[1] == trade['item'] and int(order[2]) == int(trade['requested'])
                        actual, value = int(trade['actual']), float(trade['value'])
                    if operation == 'BUY_ANIMAL':
                        record = dict(common, order_index=order_index, animal=order[1], requested=int(order[2]),
                                      actual=actual, value=value, request_status='positive_request' if parsed else 'nonpositive_ignored', cash_before_order=money,
                                      immediately_affordable={a: money >= cost for a, cost in COSTS.items()},
                                      market_action=action['market'])
                        purchases.append(record)
                        local_purchases.append(record)
                    money += value if operation == 'SELL' else -value
                assert trade_cursor == len(trades[step]), (key, step, trade_cursor, trades[step])
                assert money == replay['steps'][step + 1][0]['observation']['farms'][seat]['money'], (key, step, money)
                for event in unit_events[step]:
                    if event['operation'] != 'PLACE' or event['source'] not in ANIMALS:
                        continue
                    x, y = int(event['x']), int(event['y'])
                    positions = [own['farmer'], *own['hands']]
                    commands = [action['farmer'], *action['hands']]
                    actors = [u for u, cmd in enumerate(commands[:len(positions)]) if cmd and cmd[0] == 'PLACE' and cmd[1] == event['source'] and positions[u] == [x, y]]
                    assert actors, (key, step, event)
                    structure_events = [e for before in range(step + 1) for e in unit_events[before]
                                        if int(e['x']) == x and int(e['y']) == y and e['operation'] in ('BUILD_COOP', 'BUILD_PASTURE')]
                    recent = structure_events[-1] if structure_events else None
                    prior_occupants = []
                    last_value = None
                    for before in range(step + 1):
                        tile = replay['steps'][before][0]['observation']['farms'][seat]['tiles'][y][x]
                        value = None if tile is None else tile if isinstance(tile, str) else tile.get('animal') or tile.get('crop') or tile.get('kind')
                        if value != last_value:
                            prior_occupants.append({'step': before, 'value': value})
                            last_value = value
                    record = dict(common, animal=event['source'], x=x, y=y, worker_index_candidates=actors,
                                  center_distance=int(event['center_distance']),
                                  closer_ready_structures=int(event['closer_ready_structures']),
                                  closer_buildable_sites=int(event['closer_buildable_sites']),
                                  last_structure_step=None if recent is None else int(recent['result_index']) - 1,
                                  prior_tile_occupants=prior_occupants)
                    placements.append(record)
                    local_placements.append(record)
                prefix.update(canonical(action).encode())
                prefix.update(b'\n')
            event_streams[key] = local_purchases
            summary = dict(context, final_cash=replay['rewards'][seat], final_margin=replay['rewards'][seat] - replay['rewards'][seat ^ 1],
                           all_shops=known_shops, purchase_sequence=[{'step': e['step'], 'animal': e['animal'], 'actual': e['actual'], 'requested': e['requested'], 'order_index': e['order_index']} for e in local_purchases],
                           placement_sequence=[{'step': e['step'], 'animal': e['animal'], 'x': e['x'], 'y': e['y']} for e in local_placements],
                           total_actual_purchases={a: sum(e['actual'] for e in local_purchases if e['animal'] == a) for a in ANIMALS})
            summaries.append(summary)
    pair_candidates = []
    game_items = sorted(games)
    for i, a in enumerate(game_items):
        for b in game_items[i + 1:]:
            if a[1] != b[1] or games[a]['submission_id'] != games[b]['submission_id']:
                continue
            aa, bb = event_streams[a], event_streams[b]
            shared_actions = next((t for t, (x, y) in enumerate(zip(action_streams[a], action_streams[b])) if x != y), 719)
            for ea, eb in zip(aa, bb):
                if (ea['step'], ea['animal'], ea['requested']) == (eb['step'], eb['animal'], eb['requested']):
                    continue
                pair_candidates.append({'team': a[1], 'submission_id': int(games[a]['submission_id']), 'episode_a': a[0], 'episode_b': b[0],
                                        'shared_exact_action_prefix': shared_actions,
                                        'first_different_purchase_a': ea, 'first_different_purchase_b': eb,
                                        'same_purchase_step_and_slot': (ea['step'], ea['order_index']) == (eb['step'], eb['order_index']),
                                        'known_shops_differ': ea['shops'] != eb['shops']})
                break
    for name, records in [('purchases', purchases), ('placements', placements), ('shop_reveals', reveals), ('games', summaries)]:
        (OUT / f'{name}.json').write_text(json.dumps(records, indent=2) + '\n')
        write_csv(OUT / f'{name}.csv', records)
    (OUT / 'purchase_pair_candidates.json').write_text(json.dumps(pair_candidates, indent=2) + '\n')
    (OUT / 'SOURCES.json').write_text(json.dumps({'cohorts': COHORTS, 'input_sha256': source_hashes, 'games': len(games),
        'selection': 'Globally strong players from each frozen public leaderboard cohort; deduplicated episode/team, no local strength selection',
        'caveats': ['Different observed shops are not proof of the private branch condition', 'Immediate affordability ignores future commitments', 'Missing or failed purchase is not labeled intentional waiting', 'Recorded successful placements are from the existing audited action/transaction reconstruction']}, indent=2) + '\n')
    print('games', len(games), 'purchases', len(purchases), 'placements', len(placements), 'pairs', len(pair_candidates))


if __name__ == '__main__':
    main()
