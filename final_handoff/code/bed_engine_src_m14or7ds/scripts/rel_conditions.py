"""Per-submission strength for a fresh selection: 150 + (submission public score at fetch - 2950), clipped to [-600, 300]
(the strongest current submissions ~ the inference condition 150; weaker submissions of the same team are labeled weaker)."""
import csv, json, sys
name = sys.argv[1]
rows = json.load(open(f'data/{name}_selection.json'))['rows']
base = {(r['episode'], r['seat']): r['day_index'] for r in csv.DictReader(open(f'data/conditions_{name}.csv'))} if len(sys.argv) < 3 else {}
out = []
for r in rows:
    s = min(300.0, max(-600.0, 150.0 + r['submission_score'] - 2950.0))
    out.append((r['episode'], r['seat'], round(s, 1), base.get((str(r['episode']), str(r['seat'])), '41')))
with open(f'data/conditions_{name}_rel.csv', 'w', newline='') as f:
    w = csv.writer(f); w.writerow(['episode', 'seat', 'rating', 'day_index']); w.writerows(out)
import numpy as np
v = np.array([o[2] for o in out]); print(name, len(out), 'strength mean %.0f, share >= 100: %.2f, <= -300: %.2f' % (v.mean(), (v >= 100).mean(), (v <= -300).mean()))
