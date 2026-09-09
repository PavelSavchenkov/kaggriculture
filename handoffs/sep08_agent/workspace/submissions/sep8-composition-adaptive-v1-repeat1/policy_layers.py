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
