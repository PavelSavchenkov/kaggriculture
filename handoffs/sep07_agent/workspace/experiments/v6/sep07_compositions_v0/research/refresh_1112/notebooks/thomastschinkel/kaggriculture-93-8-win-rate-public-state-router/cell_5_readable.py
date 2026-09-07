"""Six day replay portfolio. Runtime uses observations and embedded public tapes only.

Generated assets are inserted by build_submission.py. This source is original;
action data is extracted from the public replays listed in provenance.json.
"""
import base64
import json
import zlib
_TAPES, _TREES = json.loads(zlib.decompress(base64.b64decode('<large string 61220 chars>')))
_ITEMS = 'WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP'.split()
_SHOPS = 'BAKERY BRUNCH_SPOT FARMERS_MARKET ICE_CREAM_SHOP PET_CAFE PIZZA_SHOP SMOOTHIE_SHOP YARN_STORE'.split()
_DEMAND = ((0, 5), (0, 3, 5), (0, 1, 2, 3), (0, 3, 6), (1, 1), (0, 2, 6), (3, 6), (7, 7))
_SESSIONS = {}

def _features(obs):
    p = int(obs['player'])
    own, rival = (obs['farms'][p], obs['farms'][1 - p])
    market = obs['market']
    x = [own['money'], rival['money'], own['money'] - rival['money']]
    x += [market['prices'].get(i, 0) for i in _ITEMS[:9]]
    x += [market['inventory'].get(i, 10000) - 10000 for i in _ITEMS[:9]]
    shops = obs['town']['unlocked_shops']
    counts = [shops.count(s) for s in _SHOPS]
    x += counts
    demand = [0] * 9
    for c, products in zip(counts, _DEMAND):
        for i in products:
            demand[i] += c
    x += demand
    for farm in (own, rival):
        counts = dict.fromkeys(_ITEMS, 0)
        yields = dict.fromkeys(_ITEMS[:9], 0)
        weeds = empty = 0
        for row in farm['tiles']:
            for tile in row:
                if tile is None:
                    empty += 1
                elif isinstance(tile, dict):
                    if tile.get('kind') == 'PLANT':
                        crop = tile['crop']
                        counts[crop] += 1
                        yields[crop] += tile['yield_units']
                    elif tile.get('animal'):
                        animal = tile['animal']
                        counts[animal] += 1
                        yields[{'GOOSE': 'EGG', 'COW': 'MILK', 'SHEEP': 'WOOL'}[animal]] += tile['yield_units']
                    elif tile.get('kind') == 'WEED':
                        weeds += 1
        x += [counts[_ITEMS[i]] for i in (0, 1, 2, 3, 4, 9, 10, 11)]
        x += [yields[i] for i in _ITEMS[:9]]
        x += [weeds, empty, len(farm['unlocked_quadrants'])]
    private = obs['private']
    x += [private['shed'].get(i, 0) for i in _ITEMS]
    x += [private['seeds'].get(i, 0) for i in _ITEMS[:5]]
    x += list(own['farmer'])
    return x + [0] * (100 - len(x))

def _choose(block, x):
    tree = _TREES[block]
    node = 0
    while tree[node][0] >= 0:
        feature, left, right, _, threshold = tree[node]
        node = left if x[feature] <= threshold else right
    return tree[node][3]

def agent(observation, configuration=None):
    """Kaggle callable; maintain independent route choices for both self play seats."""
    p = int(observation['player'])
    t = int(observation.get('step', observation['day'] * 24 + observation['hour']))
    if not 0 <= t < 719:
        return {'farmer': ['PASS'], 'hands': [], 'market': []}
    state = _SESSIONS.get(p)
    if state is None or t == 0 or t < state[0]:
        state = [-1, _choose(0, _features(observation))]
        _SESSIONS[p] = state
    if t % 144 == 0:
        state[1] = _choose(t // 144, _features(observation))
    state[0] = t
    source = _TAPES[state[1]][t]
    farmer = list(source['farmer'])
    hands = [list(a) for a in source['hands']]
    market = [list(a) for a in source['market']]
    return {'farmer': farmer, 'hands': hands, 'market': market}
if __name__ == '__main__':
    import sys
    for line in sys.stdin:
        if line.strip():
            request = json.loads(line)
            print(json.dumps(agent(request.get('observation', request), request.get('configuration'))), flush=True)