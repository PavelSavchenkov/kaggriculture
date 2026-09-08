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
