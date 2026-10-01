"""Training dir for fc3 (Sep 30, BC): build_train.py's data with every judge-bed world out of training (Weaknesses' request): episodes in
work/sep29_validation/swap/list_g3w*.txt and list_exact*.txt get split 'excl_bed' (all targets). fc3n (train_fc3.sh) also drops the
recent_ours rows with fc_tf_intra.py --exclude.
usage: build_train2.py <out dir>"""
import glob
import os
import sys
import pandas as pd

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_train  # noqa: E402

SWAP = '/home/pavel/Programming/kaggriculture/work/sep29_validation/swap'


def main():
    out = sys.argv[1]
    build_train.main()
    beds = set()
    for f in glob.glob(f'{SWAP}/list_g3w*.txt') + glob.glob(f'{SWAP}/list_exact*.txt'):
        beds |= {os.path.basename(l.split()[0]).split('.')[0] for l in open(f) if l.strip()}
    t = pd.read_csv(f'{out}/targets.csv', dtype={'episode': str})
    hit = t.episode.isin(beds) & (t.split == 'train')
    t.loc[hit, 'split'] = 'excl_bed'
    t.to_csv(f'{out}/targets.csv', index=False)
    print(f'{len(beds)} bed episodes; {hit.sum()} training sequences -> excl_bed')
    print(t.groupby(['source', 'split']).size().unstack(fill_value=0).to_string())


if __name__ == '__main__':
    main()
