"""Report for scripts/flex.sh: per setting, wins and paired margin change vs base; per group
(n_* network pushes, c_* compiler variants), the per-game best of the group's settings
(an oracle picking one setting per whole game) and how many base losses it flips.

usage: flex_report.py [reports/flex]
"""
import glob
import json
import sys
from pathlib import Path


def load(directory):
    games = {}
    for path in glob.glob(f"{directory}/lb/*.json"):
        r = json.load(open(path))
        if "error" not in r:
            games[Path(path).stem] = r["margin"]
    return games


def main():
    root = Path(sys.argv[1] if len(sys.argv) > 1 else "reports/flex")
    runs = {p.name: load(p) for p in sorted(root.iterdir()) if p.is_dir()}
    base = runs.pop("base")
    print(f"base: {len(base)} games, wins {sum(m > 0 for m in base.values())}, mean {sum(base.values()) / len(base):+,.0f}")
    for name, games in runs.items():
        keys = [k for k in games if k in base]
        if not keys:
            continue
        diff = sum(games[k] - base[k] for k in keys) / len(keys)
        print(f"{name:18s} n {len(keys):3d}  wins {sum(games[k] > 0 for k in keys):3d} vs base {sum(base[k] > 0 for k in keys):3d}"
              f"  paired margin {diff:+8,.0f}")
    for group in ("n_", "c_"):
        members = [g for n, g in runs.items() if n.startswith(group)]
        keys = [k for k in base if all(k in g for g in members)]
        if not members or not keys:
            continue
        best = {k: max([base[k]] + [g[k] for g in members]) for k in keys}
        losses = [k for k in keys if base[k] <= 0]
        print(f"group {group}* ({len(members)} settings, {len(keys)} games): best-of wins {sum(best[k] > 0 for k in keys)} "
              f"vs base {sum(base[k] > 0 for k in keys)}; mean best {sum(best.values()) / len(keys):+,.0f} vs base "
              f"{sum(base[k] for k in keys) / len(keys):+,.0f}; base losses flipped {sum(best[k] > 0 for k in losses)}/{len(losses)}")


if __name__ == "__main__":
    main()
