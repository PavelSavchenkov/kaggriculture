"""Sequential paired verdict (team rule 9): decide a candidate vs its baseline with the fewest games. Most ideas lose, so the rule stops
early for futility.

Paired games = the same world / seed / seat in both arms. At every checkpoint (n = 24, 48, 96, 192, then all) it reads the paired
differences d (candidate - baseline, $ per game) and says:
  REJECT   if mean(d) + 1.28 SE < delta   (an effect of delta or more is unlikely: ~90% one-sided)
  PROMOTE  if mean(d) - 2.0 SE > 0        (positive; 2.0 not 1.64 because up to 5 looks: ~5% overall) -> next stage
  CONTINUE otherwise (run more games; interleave both arms so pairs complete together)
delta = the minimal effect worth having, stated before the run (default $300 per game). With a paired SD of ~4k, a true -0.5k idea is
rejected after ~48 games and a true +0.5k one promoted after ~150-200, instead of 1,300-1,800 games on four beds.
Also prints wins / ties / losses and the half-tie score change for bed rows that have both seats' money.

Inputs: two sets of per-game CSV files (globs). Each row needs the key columns and the value column.
  duel_mm / G3 rows:  --key trace --value margin
  league rows:        --key seed,seat --value margin --file-key   (adds the opponent from '<arm>__<opp>_<block>.csv')
  --max N: the planned number of paired games (seqwatch.sh); only the fixed checkpoints and N are read, so partial counts in between
  are not extra looks. Without --max the current count is read as a final checkpoint.
  --order LIST: fixed cohorts (astra-011). LIST = the planned run order (first column = trace path or name, matched by basename);
  checkpoint n reads the first n eligible games of LIST and only once all of them are paired, so a look never uses whichever games
  happened to finish first. --only LIST: eligibility filter (e.g. the clean 99). Use both for every Stage-2 read.
usage: seqtest.py --a '<candidate glob>' --b '<baseline glob>' [--key trace] [--value margin] [--delta 300] [--file-key] [--max N]
                  [--order LIST] [--only LIST]
"""
import argparse
import glob
import math
import os
import re
import csv

p = argparse.ArgumentParser()
p.add_argument("--a", required=True)
p.add_argument("--b", required=True)
p.add_argument("--key", default="trace")
p.add_argument("--value", default="margin")
p.add_argument("--delta", type=float, default=300.0)
p.add_argument("--file-key", action="store_true")
p.add_argument("--max", type=int, default=0)
p.add_argument("--order", default="")
p.add_argument("--only", default="")
args = p.parse_args()
keys = args.key.split(",")


def load(pattern):
    rows = {}
    for f in sorted(glob.glob(pattern)):
        extra = ""
        if args.file_key:
            m = re.match(r".*__(.+)\.csv$", os.path.basename(f))
            extra = m.group(1) if m else os.path.basename(f)
        for r in csv.DictReader(open(f)):
            try:
                rows[(extra,) + tuple(r[k] for k in keys)] = float(r[args.value])
            except (KeyError, ValueError):
                continue
    return rows


a, b = load(args.a), load(args.b)
common = sorted(set(a) & set(b))
if not common:
    raise SystemExit(f"no paired games (a {len(a)} rows, b {len(b)} rows)")


def names(path):
    return [os.path.basename(line.split()[0]) for line in open(path) if line.strip() and not line.startswith("#")]


if args.order:  # fixed cohort: the list order, eligible games only; a checkpoint needs every earlier game of the list paired
    by_name = {os.path.basename(k[-len(keys)]): k for k in common}
    eligible = set(names(args.only)) if args.only else None
    planned = [n for n in names(args.order) if eligible is None or n in eligible]
    prefix = []
    for n in planned:
        if n not in by_name:
            break
        prefix.append(by_name[n])
    if args.max:
        args.max = min(args.max, len(planned))
    print(f"fixed cohort: {len(prefix)} of {len(planned)} planned games paired in list order ({len(common)} paired in total)")
    common = prefix
    if not common:
        raise SystemExit("no complete cohort prefix yet")
d = [a[k] - b[k] for k in common]


def summary(x):
    n = len(x)
    m = sum(x) / n
    sd = math.sqrt(sum((v - m) ** 2 for v in x) / (n - 1)) if n > 1 else float("inf")
    return n, m, sd / math.sqrt(n)


verdict = "CONTINUE"
checkpoints = [c for c in [24, 48, 96, 192] if not args.max or c < args.max] + [args.max or len(d)]
for n_cp in checkpoints:
    if n_cp > len(d):
        break
    n, m, se = summary(d[:n_cp])
    if m + 1.28 * se < args.delta:
        verdict = f"REJECT at n={n}"
        break
    if m - 2.0 * se > 0:
        verdict = f"PROMOTE at n={n}"
        break
if verdict == "CONTINUE" and args.max and len(d) >= args.max:
    verdict = f"max reached at n={len(d)} (no verdict: effect below what {args.max} games resolve)"
n, m, se = summary(d)
wins = sum(1 for v in d if v > 0)
ties = sum(1 for v in d if v == 0)
print(f"paired n {n}: mean {m:+.0f} $/game (SE {se:.0f}); candidate better in {wins}, equal in {ties}, worse in {n - wins - ties}")
print(f"delta {args.delta:+.0f}: {verdict}" + ("" if verdict != "CONTINUE" else f"  (need more games: have {n}; next checkpoint {next((c for c in [24, 48, 96, 192] if c > n), 2 * n)})"))
