inv0 = {item: p['I0'] for item, p in PARAMS.items()}
empty = [{}, {}]
_, inv1, money1, shed1 = replay_market_turn([[['BUY_PRODUCT', 'FERTILIZER', 4]], []], inv0, [3000, 3000], [0, 0], [1, 1], empty, PARAMS)
_, inv2, money2, _ = replay_market_turn([[['SELL', 'FERTILIZER', 4]], []], inv1, money1, [0, 0], [1, 1], shed1, PARAMS)
print(f"buy  4 fertilizer: {money1[0] - 3000:+.0f}   market inventory {inv0['FERTILIZER']} -> {inv1['FERTILIZER']}")
print(f"sell 4 fertilizer: {money2[0] - money1[0]:+.0f}   market inventory {inv1['FERTILIZER']} -> {inv2['FERTILIZER']}")
print(f'net:               {money2[0] - 3000:+.0f}')
rows_a, _, money_a, _ = replay_market_turn([[['SELL', 'CARROT', 5]], []], inv0, [3000, 3000], [0, 0], [1, 1], empty, PARAMS)
rows_b, _, money_b, _ = replay_market_turn([[['BUY_ANIMAL', 'GOOSE', 1]], []], inv0, [3000, 3000], [0, 0], [1, 1], [{'WHEAT': 100}, {}], PARAMS, shed_capacity=100)
print(f'sell 5 carrot, empty shed: {money_a[0] - 3000:+.0f}   rows={rows_a[0]}')
print(f'buy 1 goose, full shed:    {money_b[0] - 3000:+.0f}   rows={rows_b[0]}')
