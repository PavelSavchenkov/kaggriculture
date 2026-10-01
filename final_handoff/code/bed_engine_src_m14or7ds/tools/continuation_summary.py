"""Final-margin delta of market continuations vs the replay, by start day, with
episode-clustered bootstrap 95% CIs. usage: continuation_summary.py out.csv [...]"""
import csv
import random
import sys
from collections import defaultdict


def clustered_ci(pairs, draws=2000):
    by_episode = defaultdict(list)
    for episode, value in pairs:
        by_episode[episode].append(value)
    groups = list(by_episode.values())
    rng = random.Random(0)
    means = []
    for _ in range(draws):
        sample = [v for g in (rng.choice(groups) for _ in groups) for v in g]
        means.append(sum(sample) / len(sample))
    means.sort()
    return means[int(0.025 * draws)], means[int(0.975 * draws)]


for path in sys.argv[1:]:
    rows = list(csv.DictReader(open(path)))
    print(path)
    for start in sorted({int(r['start_day']) for r in rows}):
        sel = [r for r in rows if int(r['start_day']) == start]
        delta = [(r['episode'], float(r['margin']) - float(r['replay_margin'])) for r in sel]
        own = [float(r['own_cash']) - float(r['replay_cash']) for r in sel]
        opp = [float(r['opponent_cash']) - float(r['replay_opponent_cash']) for r in sel]
        low, high = clustered_ci(delta)
        mean = sum(v for _, v in delta) / len(delta)
        wins = sum(v > 0 for _, v in delta)
        fails = sum(int(r['unit_failed']) - int(r['replay_unit_failed']) for r in sel)
        print(f"  start {start:2d} n={len(sel):3d} margin delta {mean:8.1f} CI [{low:8.1f}, {high:8.1f}] "
              f"wins {wins}/{len(sel)} own {sum(own)/len(own):8.1f} opp {sum(opp)/len(opp):8.1f} "
              f"extra unit fails {fails} discarded {sum(int(r['discarded']) for r in sel)} vs {sum(int(r['replay_discarded']) for r in sel)}")
