import base64
import json
import math
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


def _base_action(observation, configuration=None):
    obs = observation
    player = obs['player']
    day, hour = obs['day'], obs['hour']
    step = day * 24 + hour
    if step == 0:
        _STATE[player] = {'chosen': [10] * len(_PLANS), 'active': [False] * len(_PLANS), 'day': None, 'shop_day': None, 'context_day': None, 'selected_entry': -1, 'entry_used_today': False, 'parent_mode': False, 'decisions': 0, 'waits': 0, 'scores': [0.0] * 4}
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
    return units, orders


def _shape(kind, x, scale):
    x = max(0.0, x)
    if kind == 0:
        return x
    if kind == 1:
        return x * x
    if kind == 2:
        return math.sqrt(x)
    if kind == 3:
        return math.log(1.0 + x)
    if kind == 4:
        return math.log10(1.0 + x)
    u = x / scale
    excess = max(0.0, u - 1.0)
    return u + 8.0 * excess * excess


def _quote(item, inventory):
    base, initial, scale, below_kind, below_target, above_kind, above_target = _DATA['market'][item]
    below = inventory < initial
    distance = abs(inventory - initial)
    curve, target = (below_kind, below_target) if below else (above_kind, above_target)
    price = base + (1.0 if below else -1.0) * target * base * _shape(curve, distance, scale) / _shape(curve, scale, scale)
    return max(1, round(price))


def _forecast_inputs(obs):
    day = obs['day']
    rival = [[0.0] * 3 for _ in range(30)]
    for row in obs['farms'][obs['player'] ^ 1]['tiles']:
        for tile in row:
            if not isinstance(tile, dict) or not tile.get('animal'):
                continue
            first, interval, cap, product = _DATA['animals'][_ITEM_INDEX[tile['animal']] - 9]
            placed, pending = tile.get('placed_day', 0), tile.get('pending_care_bonus', 0)
            for future in range(day + 1, 30):
                if future >= placed + first and (future - placed - first) % interval == 0:
                    rival[future][product - 5] += min(cap, 1 + pending)
                    pending = 0
                pending += 1
    known, expected = [1.0] * 9, [0.0] * 9
    shops = obs['town']['unlocked_shops']
    for product in range(9):
        for shop in shops:
            i = _SHOP_INDEX[shop]
            if _DATA['shop_mask'][i] & (1 << product):
                known[product] += 6 * _DATA['shop_mult'][i]
        for shop in range(8):
            if _DATA['shop_mask'][shop] & (1 << product):
                expected[product] += 6.0 * _DATA['shop_mult'][shop] / 8
    return rival, known, expected


def _value(obs, model, entry, inputs):
    rival, known, expected = inputs
    stock = [float(obs['market']['inventory'][item]) for item in _ITEMS[:9]]
    own_value = rival_value = 0.0
    observed = len(obs['town']['unlocked_shops'])
    for day in range(obs['day'], 30):
        output, wheat = (entry['biology'][day][0], entry['biology'][day][1]) if entry else ([0] * 9, 0)
        for product in range(9):
            unseen = max(0, min(8, day // 3) - observed)
            demand = known[product] + 0.35 * unseen * expected[product]
            sells = float(model['sales'][day][product] + output[product])
            buys = float(model['buys'][day][product] + (wheat if product == 0 else 0))
            internal = min(sells, buys)
            sells -= internal
            buys -= internal
            other = rival[day][product - 5] if 5 <= product <= 7 else 0.0
            middle = stock[product] + 0.5 * (sells + other - buys - demand)
            sell_price, buy_price = _quote(product, middle), _quote(product, middle - 1)
            own_value += sells * sell_price - buys * buy_price
            rival_value += other * sell_price
            stock[product] -= demand + buys
            if sell_price > 1:
                stock[product] += sells + other
    if entry:
        own_value -= entry['animal_cost']
    return own_value - rival_value


def _guard(state, key, plans, own, private, day, hour, live, units, orders):
    if hour == 0:
        state[key] = next((p for p in plans if p['day'] == day and _matches(p, own, private, day)), None)
    if state[key]:
        return _decode(state[key]['actions'][hour], live)
    return units, orders


def _deferred(obs, own, private, state, units, orders):
    day, hour = obs['day'], obs['hour']
    step = day * 24 + hour
    if state['parent_mode']:
        return units, orders
    entries = _DATA['entries']
    if hour == 0:
        state['entry_used_today'] = False
        if state['selected_entry'] < 0:
            context = next((e['model'] for e in entries if e['plan']['day'] == day and _matches(e['plan'], own, private, day)), -1)
            if context < 0 and day == 11:
                state['parent_mode'] = True
                return units, orders
            if context >= 0:
                state['decisions'] += 1
                state['scores'] = [-1e100, -1e100, -1e100, 0.0]
                model = _DATA['models'][context]
                inputs = _forecast_inputs(obs)
                baseline = _value(obs, model, None, inputs)
                best, winner = 0.0, -1
                for i, entry in enumerate(entries):
                    entry_day = entry['plan']['day']
                    if entry['model'] != context or entry_day < day:
                        continue
                    gain = _value(obs, model, entry, inputs) - baseline
                    if entry_day == day:
                        state['scores'][entry['item'] - 9] = gain
                    else:
                        state['scores'][3] = max(state['scores'][3], gain)
                    if gain > best:
                        best, winner = gain, i
                if winner >= 0 and entries[winner]['plan']['day'] == day:
                    state['selected_entry'] = winner
                    state['entry_used_today'] = True
                else:
                    state['waits'] += 1
    if step == 265:
        assert orders[7] == [5, 9, 1], (step, orders)
        orders[7] = [0, 0, 0]
    positions = [own['farmer'], *own['hands']]
    for u, action in enumerate(units):
        x, y = positions[u]
        if day == 11 and action[0] == 5 and action[1] == 9 and not private['shed'].get('GOOSE', 0):
            units[u] = action = [0, 0, 1]
        if y * 10 + x != 32:
            continue
        if day == 11 and action[0] == 7 and action[1] == 9:
            units[u] = action = [0, 0, 1]
        tile = own['tiles'][y][x]
        if isinstance(tile, dict) and not tile.get('animal') and tile.get('kind') in ('COOP', 'PASTURE') and action[0] in (15, 17, 16, 10):
            units[u] = [0, 0, 1]
    selected = state['selected_entry']
    if state['entry_used_today']:
        return _decode(entries[selected]['plan']['actions'][hour], len(units))
    if selected >= 0 and entries[selected]['item'] != 9:
        product = _DATA['animals'][entries[selected]['item'] - 9][3]
        for u, action in enumerate(units):
            if action[0] == 7 and action[1] == 5 and private['inventories'][u].get(_ITEMS[product], 0) > 0:
                units[u] = [6, 0, 0]
        if any(o[0] == 6 and o[1] == 5 for o in orders) and not any(o[0] == 6 and o[1] == product for o in orders) and len(orders) < 10:
            orders.append([6, product, 100])
    return units, orders


def agent(observation, configuration=None):
    obs = observation
    units, orders = _base_action(obs, configuration)
    state = _STATE[obs['player']]
    own, private = obs['farms'][obs['player']], obs['private']
    day, hour, live = obs['day'], obs['hour'], len(own['hands']) + 1
    units, orders = _guard(state, 'shop_day', _DATA['shop_days'], own, private, day, hour, live, units, orders)
    units, orders = _deferred(obs, own, private, state, units, orders)
    units, orders = _guard(state, 'context_day', _DATA['context_days'], own, private, day, hour, live, units, orders)
    actions = [[_OPS[op], _ITEMS[item], n] if op in (5, 7) else [_OPS[op], _ITEMS[item]] if op == 8 else [_OPS[op]] for op, item, n in units]
    market = [[_MARKET[op]] if op <= 2 else [_MARKET[op], _ITEMS[item], n] for op, item, n in orders]
    return {'farmer': actions[0], 'hands': actions[1:], 'market': market}
