"""d3 for fc3v / fc3nv (Sep 30, BC; astra 14:30): d2 (bed worlds out of TRAIN) with a clean checkpoint-selection set. val_recent rows of
recent_ours and of every judge-bed episode become 'val_drop' (neither trained nor used for early stopping). Also writes ours.csv (team
"My second life", our Kaggle team: 2 old TRAIN rows) for --exclude-teams. Rows are symlinked from d2.
usage: build_train3.py <d2 dir> <out dir>"""
import glob
import os
import sys
import pandas as pd

SWAP = '/home/pavel/Programming/kaggriculture/work/sep29_validation/swap'


def main():
    src, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    for f in sorted(glob.glob(f'{src}/rows_*.csv')):
        dst = f'{out}/{os.path.basename(f)}'
        if not os.path.exists(dst):
            os.symlink(os.path.realpath(f), dst)
    beds = set()
    for f in glob.glob(f'{SWAP}/list_g3w*.txt') + glob.glob(f'{SWAP}/list_exact*.txt'):
        beds |= {os.path.basename(l.split()[0]).split('.')[0] for l in open(f) if l.strip()}
    t = pd.read_csv(f'{src}/targets.csv', dtype={'episode': str})
    drop = (t.split == 'val_recent') & ((t.source == 'recent_ours') | t.episode.isin(beds))
    t.loc[drop, 'split'] = 'val_drop'
    t.to_csv(f'{out}/targets.csv', index=False)
    pd.read_csv(f'{src}/recent.csv').to_csv(f'{out}/recent.csv', index=False)
    pd.DataFrame({'team': ['My second life']}).to_csv(f'{out}/ours.csv', index=False)
    v = t[t.split == 'val_recent']
    print(f'val_drop {drop.sum()} (recent_ours {((t.source == "recent_ours") & drop).sum()}); clean val {len(v)}: '
          + ', '.join(f'{k} {n}' for k, n in v.source.value_counts().items()))
    print(f'TRAIN rows with team "My second life": {((t.split == "train") & (t.team == "My second life")).sum()}')


if __name__ == '__main__':
    main()
