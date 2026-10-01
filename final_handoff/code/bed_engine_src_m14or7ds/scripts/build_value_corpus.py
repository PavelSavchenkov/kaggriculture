"""On-policy value-model corpus from scripts/value_data.sh output: every <opponent>/<seed>_<seat>.txt
trace becomes one corpus line for tools/extract (our seat; synthetic episode id
4_000_000_000 + 100_000 * k + 2 * seed + seat for the k-th opponent directory; seeds ending in 0
are validation), plus margins.csv (episode, seat, final margin from <opponent>.csv) and a
conditions file (strength 150, day index 41: the agent's own inference condition).
usage: build_value_corpus.py reports/value_data out_dir
"""
import csv
import sys
from pathlib import Path

root, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
lines, margins = [], []
for k, table in enumerate(sorted(root.glob("*.csv"))):
    games = {(int(r["seed"]), int(r["seat"])): float(r["margin"]) for r in csv.DictReader(open(table))}
    for (seed, seat), margin in sorted(games.items()):
        trace = root / table.stem / f"{seed}_{seat}.txt"
        if not trace.exists():
            continue
        episode = 4_000_000_000 + 100_000 * k + 2 * seed + seat
        split = "validation" if seed % 10 == 0 else "train"
        lines.append(f"{episode} {seat} 0 0 {split} {trace}\n")
        margins.append((episode, seat, margin))
(out / "corpus.txt").write_text("".join(lines))
with open(out / "margins.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["episode", "seat", "margin"])
    w.writerows(margins)
with open(out / "conditions.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["episode", "seat", "rating", "day_index"])
    w.writerows((e, s, "150.0", 41) for e, s, _ in margins)
print(len(lines), "perspectives")
