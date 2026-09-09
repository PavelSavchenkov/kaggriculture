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


def _investment_action(observation, configuration=None):
    obs = observation
    units, orders = _base_action(obs, configuration)
    state = _STATE[obs['player']]
    own, private = obs['farms'][obs['player']], obs['private']
    day, hour, live = obs['day'], obs['hour'], len(own['hands']) + 1
    units, orders = _guard(state, 'shop_day', _DATA['shop_days'], own, private, day, hour, live, units, orders)
    units, orders = _deferred(obs, own, private, state, units, orders)
    units, orders = _guard(state, 'context_day', _DATA['context_days'], own, private, day, hour, live, units, orders)
    return units, orders

# Deployment of the frozen C++ crop, portfolio, wool and sale controllers.
# All plan data is immutable; each seat owns its episode state.
_CURRENT = [None, None]
_MASK64 = (1 << 64) - 1


def _clone(value):
    if isinstance(value, dict):
        if 'actions' in value:
            return value
        return {k: _clone(v) for k, v in value.items()}
    if isinstance(value, list):
        return [_clone(v) for v in value]
    return value


def _demand(obs, product):
    return sum(_DATA['shop_mult'][_SHOP_INDEX[s]] for s in obs['town']['unlocked_shops']
               if _DATA['shop_mask'][_SHOP_INDEX[s]] & (1 << product))


def _sequence(obs, state, plans, action):
    own, private = obs['farms'][obs['player']], obs['private']
    day, hour = obs['day'], obs['hour']
    if hour == 0:
        if day == plans[0]['day']:
            state['entered'] = _matches(plans[0], own, private, day)
        state['plan'] = next((p for p in plans if state.get('entered') and p['day'] == day
                              and _matches(p, own, private, day)), None)
    if state.get('plan'):
        return _decode(state['plan']['actions'][hour], len(own['hands']) + 1)
    return action


def _scenario(obs, sample):
    shops = [_SHOP_INDEX[s] for s in obs['town']['unlocked_shops']]
    for reveal in range(len(shops), 8):
        permutation = list(range(8))
        state = (((sample // 8 + 1) * 0xd1b54a32d192ed03) ^ ((reveal + 1) * 0x94d049bb133111eb)) & _MASK64
        for i in range(7, 0, -1):
            state = (state + 0x9e3779b97f4a7c15) & _MASK64
            x = ((state ^ (state >> 30)) * 0xbf58476d1ce4e5b9) & _MASK64
            x = ((x ^ (x >> 27)) * 0x94d049bb133111eb) & _MASK64
            j = (x ^ (x >> 31)) % (i + 1)
            permutation[i], permutation[j] = permutation[j], permutation[i]
        shops.append(permutation[sample % 8])
    berry = sum(_DATA['shop_mult'][s] for s in shops[:6] if _DATA['shop_mask'][s] & 8) >= 4
    demand = [[float(p != 8) + sum(6 * _DATA['shop_mult'][s] for s in shops[:min(8, day // 3)]
                  if _DATA['shop_mask'][s] & (1 << p)) for p in range(9)] for day in range(30)]
    return int(berry), demand


def _flow_value(obs, course, demand, rival):
    stock = [float(obs['market']['inventory'][item]) for item in _ITEMS[:9]]
    own_value = rival_value = 0.0
    for day in range(obs['day'], 30):
        for product in range(9):
            sells, buys = float(course['sales'][day][product]), float(course['buys'][day][product])
            internal = min(sells, buys)
            sells -= internal
            buys -= internal
            other = rival[day][product - 5] if 5 <= product <= 7 else 0.0
            middle = stock[product] + 0.5 * (sells + other - buys - demand[day][product])
            sell_price, buy_price = _quote(product, middle), _quote(product, middle - 1)
            own_value += sells * sell_price - buys * buy_price
            rival_value += other * sell_price
            stock[product] -= demand[day][product] + buys
            if sell_price > 1:
                stock[product] += sells + other
    return own_value, own_value - rival_value


def _common(obs, cfg, state):
    player = obs['player']
    _STATE[player] = state.get('legacy')
    action = _investment_action(obs, cfg)
    state['legacy'] = _STATE[player]
    own, private = obs['farms'][player], obs['private']
    day, hour, step = obs['day'], obs['hour'], obs['day'] * 24 + obs['hour']
    if hour == 0 and day == 20:
        state['berry'] = _demand(obs, 3) >= 4
    fert = _sequence(obs, state['fert'], _DATA['fert_days'], action)
    if state.get('berry'):
        action = fert
    leaf = int(state.get('berry', False))
    wheat = _sequence(obs, state['wheat'], _DATA['wheat'][leaf], action)
    rotation = _sequence(obs, state['rotation'], _DATA['rotation'][leaf], action)
    if day == 12 and hour == 0:
        tomatoes = _demand(obs, 2)
        state['rotation_selected'] = tomatoes >= 2 and bool(state['rotation'].get('plan'))
        state['wheat_context'] = tomatoes < 2
    action = rotation if state.get('rotation_selected') else wheat
    units, orders = action
    if step == 0 and len(orders) <= 8:
        for m in orders:
            if m[:2] == [4, 0]:
                state['opening_shift'] = m[2] - 13
                m[2] = 13
        orders = [[4, 0, 32], [6, 0, 32]] + orders
    elif step == 1:
        for m in orders:
            if m[:2] == [6, 0]:
                m[2] = max(0, m[2] - state.get('opening_shift', 0))
                break
    if day == 13 and hour == 0:
        courses = _DATA['portfolio']
        eligible = [bool(state.get('wheat_context')) and _matches(c[0]['days'][0], own, private, day) for c in courses]
        gains, squares = [0.0] * 4, [0.0] * 4
        if state.get('wheat_context'):
            rival = _forecast_inputs(obs)[0]
            for sample in range(32):
                future_leaf, demand = _scenario(obs, sample)
                baseline = courses[0][future_leaf]
                before = _flow_value(obs, baseline, demand, rival)[1]
                for c in range(1, 4):
                    if eligible[c]:
                        course = courses[c][future_leaf]
                        gain = _flow_value(obs, course, demand, rival)[1] - before - (course['fixed'] - baseline['fixed'])
                        gains[c] += gain
                        squares[c] += gain * gain
        best = 0.0
        state['portfolio_choice'] = 0
        for c in range(1, 4):
            if eligible[c]:
                mean = gains[c] / 32
                value = mean - .5 * math.sqrt(max(0.0, squares[c] / 32 - mean * mean))
                if value > best:
                    best, state['portfolio_choice'] = value, c
    if hour == 0:
        state['portfolio_plan'] = None
        if state.get('portfolio_choice') and 13 <= day < 30:
            plan = _DATA['portfolio'][state['portfolio_choice']][leaf]['days'][day - 13]
            if _matches(plan, own, private, day):
                state['portfolio_plan'] = plan
    if state.get('portfolio_plan'):
        units, orders = _decode(state['portfolio_plan']['actions'][hour], len(own['hands']) + 1)
    return units, orders


def _wool_context(first, second):
    return (first == 7 and second in (3, 5, 6, 7)) or (second == 7 and first in (5, 6))


def _transfer(obs, state, rule, fallback):
    own, private = obs['farms'][obs['player']], obs['private']
    day, hour = obs['day'], obs['hour']
    step, live = day * 24 + hour, len(own['hands']) + 1
    target = _DATA['wool_target']
    if step == 144:
        eligible = (not own['hands'] and len(own['unlocked_quadrants']) == 1 and list(own['farmer']) == [4, 4]
                    and own['money'] >= 700
                    and all(not private['inventories'][0].get(item, 0) and
                        (i in (0, 8) or private['shed'].get(item, 0) == target['shed'][i]) for i, item in enumerate(_ITEMS))
                    and all(_tile_key(own['tiles'][cell // 10][cell % 10], day) == key for cell, key in target['tiles']))
        shops = [_SHOP_INDEX[s] for s in obs['town']['unlocked_shops']]
        if eligible and (rule == 0 or (_demand(obs, 7) >= 2 and _wool_context(*shops[:2]))):
            state['route'] = 1
            state['wheat'] = max(0, target['shed'][0] - private['shed'].get('WHEAT', 0))
            state['fertilizer'] = max(0, private['shed'].get('FERTILIZER', 0) - target['shed'][8])
            state['seeds'] = [max(0, target['seeds'][i] - private['seeds'].get(item, 0)) for i, item in enumerate(_ITEMS[:5])]
    if not state.get('route'):
        return fallback
    units, orders = _decode(_DATA['wool_tape'][step], live)
    if step == 144:
        for m in orders:
            if m[:2] == [4, 0]:
                m[2] += state['wheat']
                state['wheat'] = 0
            if m[0] == 3 and m[1] in (0, 1):
                m[2] += state['seeds'][m[1]]
                state['seeds'][m[1]] = 0
    if step == 145:
        for m in orders:
            if m[:2] == [6, 8]:
                m[2] += state['fertilizer']
                state['fertilizer'] = 0
                break
        for i, n in enumerate(state['seeds']):
            if n and len(orders) < 10:
                orders.append([3, i, n])
                state['seeds'][i] = 0
    if hour == 0:
        state['plan'] = next((d for d in _DATA['wool_days'] if d['day'] == day and _matches(d, own, private, day)), None)
    if state.get('plan'):
        units, orders = _decode(state['plan']['actions'][hour], live)
    return units, orders


def _rival(obs, cfg, state):
    step = obs['day'] * 24 + obs['hour']
    own, rival = obs['farms'][obs['player']], obs['farms'][obs['player'] ^ 1]
    fallback = None
    if not state.get('active') and not state['main_wool'].get('route'):
        fallback = _common(obs, cfg, state)
    action = _transfer(obs, state['main_wool'], 6, fallback) if not state.get('active') else None
    if step > 145 and not state.get('active'):
        return action
    alternative = _transfer(obs, state['alt_wool'], 0, fallback)
    if step == 144 and state['alt_wool'].get('route'):
        units, orders = action
        au, am = alternative
        state['pending'] = (len(au) == 1 and au[0][0] == 4 and len(am) == 10 and
            len(units) == 1 and units[0][0] in (1, 4) and len(orders) == 7 and
            all(m[0] == 1 for m in orders) and all(m[0] == 1 for m in am[:6]))
        if state['pending']:
            state['farmer'] = units[0][0]
            state['deferred'] = [m[:] for m in am[6:]]
            state['old_market'] = [obs['market']['inventory'][i] for i in _ITEMS[:9]]
            state['old_money'], state['old_hires'] = rival['money'], rival['hires_today']
    if step == 145 and state.get('pending'):
        old = state['old_market']
        center = cfg.get('centerSellInterval', 24) if cfg else 24
        interval = cfg.get('shopSellInterval', 4) if cfg else 4
        demand = [int(i != 8 and 144 % center == 0) + (_demand(obs, i) if 144 % interval == 0 else 0) for i in range(9)]
        flow = [obs['market']['inventory'][item] - old[i] + demand[i] for i, item in enumerate(_ITEMS[:9])]
        hires = rival['hires_today']
        cost = sum(_fib(i) for i in range(state['old_hires'], hires)) * (cfg.get('hireCostMultiplier', 1) if cfg else 1)
        extra = state['old_money'] - rival['money'] - cost - _quote(0, old[0] - 1)
        first, second = [_SHOP_INDEX[s] for s in obs['town']['unlocked_shops'][:2]]
        broaden = (7 in (first, second) and len(rival['hands']) == 6 and state['old_hires'] == 0 and hires == 6
                   and flow[0] == -1 and all(v == 0 for v in flow[1:]) and extra > 0)
        state['active'] = _wool_context(first, second) or broaden
        if state['active']:
            units, orders = alternative
            assert len(own['hands']) == 7 and list(own['hands'][6]) == [4, 5] and len(orders) <= 6
            units[0] = [4 if state['farmer'] == 1 else 1, 0, 1]
            units[7] = [3, 0, 1]
            hire = next(i for i, m in enumerate(orders) if m[0] == 1)
            orders[hire] = [0, 0, 0]
            orders.extend(m[:] for m in state['deferred'])
    return alternative if state.get('active') else action


def _fib(index):
    a, b = 1, 1
    for _ in range(index):
        a, b = b, a + b
    return a


def _projected(obs, units):
    own, private = obs['farms'][obs['player']], obs['private']
    stock = [private['shed'].get(item, 0) for item in _ITEMS]
    total = sum(stock)
    for u, (x, y) in enumerate([own['farmer'], *own['hands']]):
        if x not in (4, 5) or y not in (4, 5):
            continue
        op, item, n = units[u]
        inv = private['inventories'][u]
        if op == 5:
            moved = min(stock[item], max(0, n))
            stock[item] -= moved
            total -= moved
        elif op == 6:
            for key, quantity in inv.items():
                item = _ITEM_INDEX[key]
                moved = min(quantity, max(0, 100 - total))
                stock[item] += moved
                total += moved
        elif op == 7 and item < 9:
            moved = min(max(0, n), inv.get(_ITEMS[item], 0), max(0, 100 - total))
            stock[item] += moved
            total += moved
    return stock


def _suppress(orders, state, step):
    if state.get('due') == step:
        for m in orders:
            if m[0] == 6:
                n = min(max(0, m[2]), state['suppression'][m[1]])
                m[2] -= n
                state['suppression'][m[1]] -= n
    state['due'], state['suppression'] = -1, [0] * 12


def _advance(obs, units, orders, next_orders, state):
    stock, sales = _projected(obs, units), [0] * 12
    already = {m[1] for m in orders if m[0] == 6}
    for op, item, n in next_orders:
        if op == 6:
            sales[item] += max(0, n)
    for item in range(1, 8):
        if len(orders) == 10:
            break
        if item in already or obs['market']['prices'][_ITEMS[item]] < 2:
            continue
        n = min(stock[item], sales[item])
        if n > 0:
            orders.append([6, item, n])
            state['suppression'][item] = n
            state['due'] = obs['day'] * 24 + obs['hour'] + 1


def _accepted(obs, cfg, state):
    units, orders = _rival(obs, cfg, state['rival'])
    step = obs['day'] * 24 + obs['hour']
    lead = state['lead']
    _suppress(orders, lead, step)
    if 216 <= step < 718 and obs['hour'] < 23 and step % 4:
        stock = _projected(obs, units)
        already = {m[1] for m in orders if m[0] == 6}
        if len(orders) < 10 and any(stock[i] > 0 and i not in already and obs['market']['prices'][_ITEMS[i]] >= 2 for i in range(1, 8)):
            following = dict(obs, hour=obs['hour'] + 1)
            shadow = _clone(state['rival'])
            _, next_orders = _rival(following, cfg, shadow)
            _advance(obs, units, orders, next_orders, lead)
    orders = [m for m in orders if not (step >= 216 and m[0] == 6 and 0 < m[1] < 8 and m[2] <= 0)]
    return units, orders

def _group_choose(obs, state):
    day = obs['day']
    if state.get('family', -1) >= 0 or day not in (15, 20):
        return
    family = int(day == 20)
    data = _DATA['groups'][family]
    own, private = obs['farms'][obs['player']], obs['private']
    visible_leaf = int(_demand(obs, 3) >= 4) if family else 0
    eligible = [False] * len(data['choices'])
    for c in range(1, len(eligible)):
        course = data['choices'][c][visible_leaf]
        cash = course['fixed'][day] + 1000
        for p in range(9):
            cash += course['buys'][day][p] * obs['market']['prices'][_ITEMS[p]]
        eligible[c] = _matches(course['days'][0], own, private, day) and own['money'] >= cash
    means, squares = [0.0] * len(eligible), [0.0] * len(eligible)
    rival = _forecast_inputs(obs)[0]
    for sample in range(32):
        _, demand = _scenario(obs, sample)
        leaf = visible_leaf if family else sample % 2
        baseline = data['choices'][0][leaf]
        before = _flow_value(obs, baseline, demand, rival)[1]
        for c in range(1, len(eligible)):
            if eligible[c]:
                course = data['choices'][c][leaf]
                fixed = sum(course['fixed'][d] - baseline['fixed'][d] for d in range(day, 30))
                gain = _flow_value(obs, course, demand, rival)[1] - before - fixed
                means[c] += gain
                squares[c] += gain * gain
    best, selected = 0.0, 0
    for c in range(1, len(eligible)):
        if eligible[c]:
            mean = means[c] / 32
            score = mean - .5 * math.sqrt(max(0.0, squares[c] / 32 - mean * mean))
            if score > best:
                best, selected = score, c
    if selected:
        state.update(family=family, choice=selected, leaf=visible_leaf)


def _distance(plan, obs):
    own, private = obs['farms'][obs['player']], obs['private']
    result = 1000 * bool(own['hands'] or len(own['unlocked_quadrants']) != plan['quadrants'] or list(own['farmer']) != [4, 4])
    for i, item in enumerate(_ITEMS):
        result += int(private['shed'].get(item, 0) != plan['shed'][i]) + int(private['inventories'][0].get(item, 0) != 0)
    result += sum(private['seeds'].get(item, 0) != plan['seeds'][i] for i, item in enumerate(_ITEMS[:5]))
    result += 10 * sum(_tile_key(own['tiles'][cell // 10][cell % 10], obs['day']) != key for cell, key in plan['tiles'])
    return result


def _animal_action(obs, cfg, state):
    units, orders = _accepted(obs, cfg, state['base'])
    day, hour = obs['day'], obs['hour']
    step = day * 24 + hour
    if hour == 0:
        _group_choose(obs, state)
        state['plan'] = None
        if state.get('family', -1) >= 0:
            family, choice = state['family'], state['choice']
            data = _DATA['groups'][family]
            index = day - data['first']
            leaves = [state['leaf']] if family else [0, 1]
            leaf = min(leaves, key=lambda leaf: _distance(data['choices'][choice][leaf]['days'][index], obs))
            state['leaf'] = leaf
            plan = data['choices'][choice][leaf]['days'][index]
            own, private = obs['farms'][obs['player']], obs['private']
            if not _matches(plan, own, private, day) and family == 0 and choice == 2 and day == 23:
                if _matches(_DATA['repair'], own, private, day):
                    plan = _DATA['repair']
                    state['repaired'] = True
            if not _matches(plan, own, private, day):
                state['missed_days'] = state.get('missed_days', 0) | (1 << day)
            state['plan'] = plan
    if state.get('plan'):
        units, orders = _decode(state['plan']['actions'][hour], len(obs['farms'][obs['player']]['hands']) + 1)
        _suppress(orders, state, step)
        if step < 718 and hour < 23 and step % 4:
            _advance(obs, units, orders, state['plan']['actions'][hour + 1][1], state)
        orders = [m for m in orders if not (m[0] == 6 and 1 <= m[1] <= 7 and m[2] == 0)]
    return units, orders


def agent(observation, configuration=None):
    obs = observation
    player = obs['player']
    step = obs['day'] * 24 + obs['hour']
    if step == 0:
        _CURRENT[player] = {'base': {'rival': {'fert': {}, 'wheat': {}, 'rotation': {},
            'main_wool': {}, 'alt_wool': {}}, 'lead': {}}, 'family': -1}
    state = _CURRENT[player]
    units, orders = _animal_action(obs, configuration, state)
    if step >= 216:
        def premium(m):
            return m[0] == 6 and m[1] not in (0, 8) and m[2] > 0
        orders = [m for m in orders if premium(m)] + [m for m in orders if not premium(m)]
    if step == 0:
        assert orders[0][:2] == [4, 0] and orders[1][:2] == [6, 0]
        orders[0][2] = orders[1][2] = 24
    actions = [[_OPS[op], _ITEMS[item], n] if op in (5, 7) else [_OPS[op], _ITEMS[item]] if op == 8 else [_OPS[op]] for op, item, n in units]
    market = [[_MARKET[op]] if op <= 2 else [_MARKET[op], _ITEMS[item], n] for op, item, n in orders]
    assert len(actions) == 1 + len(obs['farms'][player]['hands']) and len(market) <= 10
    return {'farmer': actions[0], 'hands': actions[1:], 'market': market}
