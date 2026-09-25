"""Slot order of SELL orders in top-team traces: for hours with >= 2 sell orders, is the list in
fixed product order (ascending item id), and which products take the first slot?"""
import collections, random
P = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
random.seed(0)
lines = [l.split() for l in open('top_list.txt')]
sample = random.sample(lines, 400)
stats = collections.defaultdict(lambda: collections.Counter())
for f in sample:
    trace, labs = f[0], f[1:3]
    rows = open(trace).read().split('\n')
    turns = rows[3:3 + 2 * 719]
    for i, t in enumerate(turns):
        seat = i % 2
        team = labs[seat].split('|')[0]; rank = int(labs[seat].split('|')[1])
        if rank > 10: continue
        v = list(map(int, t.split()))
        nu, no = v[0], v[1]
        orders = [v[2 + 3 * nu + 3 * k: 5 + 3 * nu + 3 * k] for k in range(no)]
        sells = [o[1] for o in orders if o[0] == 6 and o[2] > 0]
        if len(sells) < 2: continue
        c = stats[team]
        c['hours'] += 1
        c['ascending'] += sells == sorted(sells)
        c['first_' + P[sells[0]] if sells[0] < 9 else 'first_?'] += 1
        # is the highest-base product first?
        base = [25, 35, 60, 120, 250, 50, 160, 200, 100]
        c['max_value_first'] += base[sells[0]] == max(base[s] for s in sells if s < 9)
for team, c in sorted(stats.items(), key=lambda kv: -kv[1]['hours']):
    h = c['hours']
    firsts = {k[6:]: round(v / h, 2) for k, v in c.most_common() if k.startswith('first_')}
    print(f"{team:24s} hours {h:5d} ascending {c['ascending']/h:.2f} highest-value-first {c['max_value_first']/h:.2f} first slot {dict(list(firsts.items())[:4])}")
