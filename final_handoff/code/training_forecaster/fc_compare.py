"""Compares forecasters scored by fc_eval.py on the same rows (days 12-27 by default). Per opponent group and decision hour (0 = dawn
head, 6 / 12 / 18 = intra-day head): Poisson loss of the band totals (mean per product-day over the 4 bands, lower is better; a proper
score at band resolution) and, for the thin products (strawberry / egg / milk / wool), predicted / actual per band. Summary weighted by
our live opponent mix (Sep 29-30: ranks 1-10 11%, 11-30 24%, 31-100 23%, 101+ 41%).
usage: fc_compare.py <eval_a.csv> <eval_b.csv> ...  [--days 12 27]"""
import sys
import numpy as np
import pandas as pd

MIX = {'top10': 0.11, 'r11_30': 0.24, 'r31_100': 0.23, 'r101': 0.41}
THIN = {3: 'strawb', 5: 'egg', 6: 'milk', 7: 'wool'}


def main():
    args = sys.argv[1:]
    lo, hi = 12, 27
    if '--days' in args:
        i = args.index('--days'); lo, hi = int(args[i + 1]), int(args[i + 2]); args = args[:i] + args[i + 3:]
    names = [a.rsplit('/', 1)[-1].replace('eval_', '').replace('.csv', '') for a in args]
    ds = [pd.read_csv(a) for a in args]
    ds = [d[d.day.between(lo, hi)] for d in ds]
    score = {}
    for n, d in zip(names, ds):
        for (g, dec), x in d.groupby(['group', 'decision']):
            p = np.clip(x[[f'pred_b{k}' for k in range(4)]].values, 1e-6, None); a = x[[f'act_b{k}' for k in range(4)]].values
            ok = ~np.isnan(a)
            score[(n, g, dec)] = float(np.where(ok, p - np.nan_to_num(a) * np.log(p), 0).sum(1).mean())
    groups = ['mm', 'top10', 'r11_30', 'r31_100', 'r101', 'ours']
    print(f"Poisson loss of band totals per product-day, days {lo}-{hi} (lower is better); columns: " + ' | '.join(names))
    for g in groups:
        print(f"  {g:8s} " + '   '.join(f"D{dec:<2d} " + ' '.join(f"{score[(n, g, dec)]:7.4f}" for n in names) for dec in (0, 6, 12, 18)))
    print("  live-mix " + '   '.join(f"D{dec:<2d} " + ' '.join(f"{sum(w * score[(n, g, dec)] for g, w in MIX.items()):7.4f}" for n in names) for dec in (0, 6, 12, 18)))
    nd = sorted({int(x) for d in ds for x in d.decision.unique() if x >= 100})
    if nd:
        print("\nnext-morning head (tomorrow h0-2 / h3-11 forecast at cut c): Poisson loss (both bands) and thin-product pred / actual")
        for g in groups:
            cells = []
            for dec in nd:
                cells.append(f"c{dec - 100} " + ' '.join(f"{score.get((n, g, dec), float('nan')):7.4f}" for n in names))
            print(f"  {g:8s} " + '   '.join(cells))
        print("  live-mix " + '   '.join(f"c{dec - 100} " + ' '.join(f"{sum(w * score.get((n, g, dec), float('nan')) for g, w in MIX.items()):7.4f}" for n in names) for dec in nd))
        for g in groups:
            cells = []
            for n, d in zip(names, ds):
                x = d[(d.group == g) & d['product'].isin(list(THIN)) & (d.decision == 118)]
                cells.append(f"{n}: " + '/'.join(f"{x[f'pred_b{k}'].sum() / max(x[f'act_b{k}'].sum(), 1e-9):4.2f}" if len(x) else '  - ' for k in range(2)))
            print(f"  {g:8s} thin at c18 " + '   '.join(cells))
    print(f"\nthin products, pred / actual per band h0-2 / h3-11 / h12-20 / h21-23 (dawn head, days {lo}-{hi})")
    for g in groups:
        for p, pn in THIN.items():
            cells = []
            for n, d in zip(names, ds):
                x = d[(d.group == g) & (d['product'] == p) & (d.decision == 0)]
                r = [x[f'pred_b{k}'].sum() / x[f'act_b{k}'].sum() if x[f'act_b{k}'].sum() >= 5 else np.nan for k in range(4)]
                cells.append('/'.join('  - ' if np.isnan(v) else f'{v:4.2f}' for v in r))
            print(f"  {g:8s} {pn:7s} " + '   '.join(f"{n}: {c}" for n, c in zip(names, cells)))


if __name__ == '__main__':
    main()
