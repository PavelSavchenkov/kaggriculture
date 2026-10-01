"""LB screen summary: per challenger run dir (out/<tag>) and opponent: games, W/D/L from money (challenger = money[seat]), half-draw
score, mean margin; then paired vs a control run on identical (opponent, seed, seat): wins gained / lost and the margin difference (SE).
usage: lbsum.py <control tag> <tag> ..."""
import glob, json, os, sys
import numpy as np, pandas as pd
def load(tag):
    rows = []
    for f in glob.glob(f'out/{tag}/*.json'):
        r = json.load(open(f)); m = r.get('money')
        if not m or r.get('error'): continue
        s = r['seat']; rows.append(dict(opp=r['opponent'], seed=r['seed'], seat=s, own=m[s], other=m[1 - s], margin=m[s] - m[1 - s]))
    return pd.DataFrame(rows)
short = lambda o: o.replace('pavel-bc-opus-v17d-', '').replace('pavel-', '')[:22]
ctl = load(sys.argv[1]); key = ['opp', 'seed', 'seat']
for tag in sys.argv[1:]:
    d = load(tag)
    if d.empty: print(f'{tag}: no games yet'); continue
    parts = []
    for o, x in d.groupby('opp'):
        w, l = int((x.margin > 0).sum()), int((x.margin < 0).sum()); dr = len(x) - w - l
        parts.append(f"{short(o)} {w}-{dr}-{l} ({x.margin.mean():+.0f})")
    w, l = int((d.margin > 0).sum()), int((d.margin < 0).sum())
    line = f"{tag}: n {len(d)}, W-D-L {w}-{len(d) - w - l}-{l}, score {100 * ((d.margin > 0).mean() + 0.5 * (d.margin == 0).mean()):.1f}% | " + ' | '.join(parts)
    if tag != sys.argv[1] and not ctl.empty:
        p = d.merge(ctl, on=key, suffixes=('', '_c'))
        if len(p) > 1:
            dm = p.margin - p.margin_c; gw = int(((p.margin > 0) & (p.margin_c <= 0)).sum()); lw = int(((p.margin <= 0) & (p.margin_c > 0)).sum())
            cl = (p.assign(dm=dm).groupby(['opp', 'seed']).dm.mean())  # seats of one seed are correlated (often identical): cluster by seed
            line += f" || vs {sys.argv[1]} paired n {len(p)} ({len(cl)} seeds): margin {dm.mean():+.0f} (seed-clustered SE {cl.std(ddof=1) / np.sqrt(len(cl)):.0f}), wins gained {gw} lost {lw}"
    print(line)
