"""Training dir for the forecaster redesign (Sep 30, BC): Kaggle games only.
- Old data (work/sep26_wide_losses/fcbig2, Sep 8-26): every Kaggle source (old_*, kaggle_*, top_sep18_24, fresh_sep26, boey); the
  local bench agents, synthetic and lineage (non-Kaggle) rows are dropped. Old test_top rows keep their split (not trained).
- Recent data (train_recent/rows_*.csv, Sep 27 - Sep 30 07:30 UTC, the perspectives not in eval_recent): source recent_<group>, team
  "recent" (loss weight via --weight-teams recent.csv:<w>); 10% of the recent traces (by hash) are split val_recent (early stopping).
Rows are symlinked, not copied.
usage: build_train.py <out dir>"""
import glob
import hashlib
import os
import sys
import pandas as pd

OLD = '/home/pavel/Programming/kaggriculture/work/sep26_wide_losses/fcbig2'
KAGGLE = ('old_', 'kaggle_', 'top_sep18_24', 'fresh_sep26', 'boey')


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    here = os.path.dirname(os.path.abspath(__file__))
    t = pd.read_csv(f'{OLD}/targets.csv', dtype={'episode': str})
    t = t[t.source.str.startswith(KAGGLE)]
    for f in sorted(glob.glob(f'{OLD}/rows_*.csv')):
        name = os.path.basename(f)
        if name.startswith(('rows_0', 'rows_1', 'rows_2', 'rows_3', 'rows_new_', 'rows_old_')) and not os.path.exists(f'{out}/{name}'):
            os.symlink(os.path.realpath(f), f'{out}/{name}')
    rec = []
    for f in sorted(glob.glob(f'{here}/train_recent/rows_*.csv')):
        name = 'rows_recent_' + os.path.basename(f)[5:]
        if not os.path.exists(f'{out}/{name}'):
            os.symlink(f, f'{out}/{name}')
        group = os.path.basename(f)[5:].split('_part')[0].replace('_all.csv', '').replace('.csv', '')
        x = pd.read_csv(f, usecols=['trace', 'target']).drop_duplicates()
        for tr, tg in zip(x.trace, x.target):
            base = os.path.basename(tr)
            val = int(hashlib.sha256(f'fc:{base}'.encode()).hexdigest(), 16) % 10 == 0
            rec.append({'episode': base.split('.')[0], 'target': int(tg), 'team': 'recent', 'source': f'recent_{group}', 'prio': 0.0,
                        'trace': f'x/{base}', 'split': 'val_recent' if val else 'train'})
    r = pd.DataFrame(rec)
    allt = pd.concat([t, r], ignore_index=True)
    allt.to_csv(f'{out}/targets.csv', index=False)
    pd.DataFrame({'team': ['recent']}).to_csv(f'{out}/recent.csv', index=False)
    print(allt.groupby(['source', 'split']).size().unstack(fill_value=0).to_string())


if __name__ == '__main__':
    main()
