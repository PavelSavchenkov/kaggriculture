import base64
import json
import zlib

_DATA = json.loads(zlib.decompress(base64.b85decode('__POLICY_PAYLOAD__')))
_TAPE = _DATA['tape']
_DAYS = {d['day']: d for d in _DATA['days']}
_PLANS = _DATA['plans']
_ITEMS = 'WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP'.split()
_ITEM_INDEX = {s: i for i, s in enumerate(_ITEMS)}
_SHOPS = 'BAKERY BRUNCH_SPOT FARMERS_MARKET ICE_CREAM_SHOP PET_CAFE PIZZA_SHOP SMOOTHIE_SHOP YARN_STORE'.split()
_SHOP_INDEX = {s: i for i, s in enumerate(_SHOPS)}
_KINDS = {None: 0, 'LOCKED': 1, 'WEED': 2, 'COOP': 3, 'PASTURE': 4, 'PLANT': 5}
_OPS = 'PASS NORTH SOUTH EAST WEST PICKUP DROP PLACE PLANT WATER HARVEST FERTILIZE DIG BUILD_COOP BUILD_PASTURE FEED COLLECT_FERTILIZER CARE'.split()
_MARKET = ['PASS', 'HIRE', 'BUY_LAND', 'BUY_SEED', 'BUY_PRODUCT', 'BUY_ANIMAL', 'SELL']
_STATE = [None, None]


def _tile_key(tile, day):
    if not isinstance(tile, dict):
        return [_KINDS[tile], -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    kind = _KINDS[tile.get('kind')]
    animal = tile.get('animal')
    key = [kind, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    if kind == 5 or animal:
        key[3] = day - tile.get('planted_day', tile.get('placed_day', 0))
        key[4] = tile.get('yield_units', 0)
        key[5] = tile.get('consecutive_unwatered', tile.get('consecutive_unfed', 0))
    if kind == 5:
        key[1] = _ITEM_INDEX[tile['crop']]
        key[7] = max(0, tile.get('fertilized_until_day', -1) - day + 1)
        key[8] = int(tile.get('watered_today', False))
    if animal:
        key[2] = _ITEM_INDEX[animal]
        key[6] = tile.get('pending_care_bonus', 0)
        key[9] = int(tile.get('fed_today', False))
        key[10] = int(tile.get('cared_today', False))
        key[11] = int(tile.get('fertilizer_available', False))
    return key


def _matches(plan, own, private, day):
    if own['hands'] or len(own['unlocked_quadrants']) != plan['quadrants'] or list(own['farmer']) != [4, 4]:
        return False
    inv = private['inventories'][0]
    if any(private['shed'].get(item, 0) != n or inv.get(item, 0)
           for item, n in zip(_ITEMS, plan['shed'])):
        return False
    if any(private['seeds'].get(item, 0) != n for item, n in zip(_ITEMS[:5], plan['seeds'])):
        return False
    return all(_tile_key(own['tiles'][cell // 10][cell % 10], day) == key for cell, key in plan['tiles'])


def _decode(entry, live):
    units = [list(u) for u in entry[0][:live]]
    units.extend([[0, 0, 1] for _ in range(live - len(units))])
    return units, [list(o) for o in entry[1]]


def _terminal(obs, own, private, units, orders, step, capacity):
    if step >= 708:
        centers = ((4, 4), (5, 4), (4, 5), (5, 5))
        positions = [own['farmer'], *own['hands']]
        for u, (x, y) in enumerate(positions):
            if not sum(private['inventories'][u].get(i, 0) for i in _ITEMS[:9]):
                continue
            tx, ty = min(centers, key=lambda p: abs(x - p[0]) + abs(y - p[1]))
            distance = abs(x - tx) + abs(y - ty)
            if step >= 718 - distance:
                op = 6 if not distance else 3 if x < tx else 4 if x > tx else 2 if y < ty else 1
                units[u] = [op, 0, 1]
    if step != 718:
        return
    covered = set()
    for order in orders:
        if order[0] == 6:
            order[2] = max(order[2], capacity)
            covered.add(order[1])
    visible = [private['shed'].get(i, 0) for i in _ITEMS[:9]]
    for u, (x, y) in enumerate([own['farmer'], *own['hands']]):
        if x in (4, 5) and y in (4, 5):
            op, item, n = units[u]
            if op == 6:
                for i in range(9):
                    visible[i] += private['inventories'][u].get(_ITEMS[i], 0)
            elif op == 7 and item < 9:
                visible[item] += min(n, private['inventories'][u].get(_ITEMS[item], 0))
    ranked = sorted(range(9), key=lambda i: -visible[i] * obs['market']['prices'][_ITEMS[i]])
    for i in ranked:
        if i not in covered and len(orders) < 10:
            orders.append([6, i, capacity])


def agent(observation, configuration=None):
    obs = observation
    player = obs['player']
    day, hour = obs['day'], obs['hour']
    step = day * 24 + hour
    if step == 0:
        _STATE[player] = {'chosen': [10] * len(_PLANS), 'active': [False] * len(_PLANS), 'day': None}
    state = _STATE[player]
    own, private = obs['farms'][player], obs['private']
    live = len(own['hands']) + 1
    if hour == 0:
        milk = wool = 0
        for shop in obs['town']['unlocked_shops']:
            index = _SHOP_INDEX[shop]
            mask, mult = _DATA['shop_mask'][index], _DATA['shop_mult'][index]
            if mask & (1 << 6):
                milk += mult
            if mask & (1 << 7):
                wool += mult
        for i, plan in enumerate(_PLANS):
            if day == plan['purchase'][0] // 24:
                state['chosen'][i] = 11 if wool > milk else 10
        route = _DAYS.get(day)
        state['day'] = route if route and _matches(route, own, private, day) else None
        if state['day'] and any(state['chosen'][i] != 10 and any(plan[k][0] // 24 == day for k in ('purchase', 'pickup', 'placement')) for i, plan in enumerate(_PLANS)):
            state['day'] = None
    units, orders = _decode(_TAPE[step], live)
    capacity = configuration.get('shedCapacity', 100) if configuration else 100
    _terminal(obs, own, private, units, orders, step, capacity)
    if state['day']:
        units, orders = _decode(state['day']['actions'][hour], live)
    for i, plan in enumerate(_PLANS):
        if state['chosen'][i] == 10:
            continue
        if step == plan['purchase'][0]:
            order = orders[plan['purchase'][1]]
            assert order == [5, 10, 1], (step, order)
            order[1] = 11
            state['active'][i] = True
        for key in ('pickup', 'placement'):
            at, u = plan[key]
            if step == at and u < live:
                assert units[u][0] in (5, 7) and units[u][1] == 10, (step, units[u])
                units[u][1] = 11
        at, u = plan['structure']
        if step == at and u < live:
            assert units[u][0] in (13, 14), (step, units[u])
            units[u][0] = 14
        if state['active'][i]:
            for u, unit in enumerate(units):
                if unit[0] == 7 and unit[1] == 6 and private['inventories'][u].get('WOOL', 0) > 0:
                    units[u] = [6, 0, 0]
            if any(o[0] == 6 and o[1] == 6 for o in orders) and not any(o[0] == 6 and o[1] == 7 for o in orders) and len(orders) < 10:
                orders.append([6, 7, 100])
    actions = [[_OPS[op], _ITEMS[item], n] if op in (5, 7) else [_OPS[op], _ITEMS[item]] if op == 8 else [_OPS[op]] for op, item, n in units]
    market = [[_MARKET[op]] if op <= 2 else [_MARKET[op], _ITEMS[item], n] for op, item, n in orders]
    return {'farmer': actions[0], 'hands': actions[1:], 'market': market}
