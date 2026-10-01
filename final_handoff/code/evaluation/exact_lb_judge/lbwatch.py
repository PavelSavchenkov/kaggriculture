"""Print an agent's official Local-LB entry from a rankings.json on stdin (active, rating, wins-losses, rank among active), plus the top 5.
usage: git show origin/main:data/rankings.json | lbwatch.py <agent id>"""
import json, sys
me = sys.argv[1]; r = json.load(sys.stdin); a = r['agents']; a = a if isinstance(a, dict) else {x['id']: x for x in a}
act = sorted(((v.get('rating', 0), k) for k, v in a.items() if v.get('active')), reverse=True)
for i, (rt, k) in enumerate(act[:5], 1): print(f'{i:2d} {rt:7.1f} {k}')
if me in a:
    v = a[me]; rank = next((i for i, (_, k) in enumerate(act, 1) if k == me), None)
    print(f"FOUND {me}: active {v.get('active')} rating {v.get('rating')} W-L {v.get('wins')}-{v.get('losses')} rank {rank}")
else: print(f'absent {me}')
