"""Official records of a challenger's pairs from the snapshot round_robin.json: prints 'opponent pair_index first_seed n_games' and
optionally dumps the games (seed, seat, money, result) as JSON. usage: official.py <snapshot> <challenger id> [dump.json]"""
import json, sys
snap, me = sys.argv[1], sys.argv[2]
rr = json.load(open(f'{snap}/data/matches/round_robin.json'))['pairs']
games = []
for k, p in rr.items():
    if p['challenger'] != me: continue
    seeds = sorted({g['seed'] for g in p['games']})
    print(p['opponent'], p['pair_index'], seeds[0] if seeds else None, len(p['games']))
    games += [dict(opponent=p['opponent'], seed=g['seed'], seat=g['seat'], money=g['money'], result=g['result']) for g in p['games']]
if len(sys.argv) > 3: json.dump(games, open(sys.argv[3], 'w'))
