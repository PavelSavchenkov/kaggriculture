"""Corpus and conditioning table for a fresh replay selection (scripts/collect_replays.py --name <name>):
data/corpus_<name>.txt (episode seat team submission split trace; split by the collector's episode hash) and
data/conditions_<name>.csv (strength = the submission's current public score minus the latest daily reference,
as in build_conditions.py carried forward; day_index = replay date - 2026-08-15). Only exported traces.
usage: scripts/build_fresh_corpus.py <name>"""
import csv, json, sys
from datetime import date
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
name = sys.argv[1]
rows = json.loads((ROOT / f"data/{name}_selection.json").read_text())["rows"]
daily = {(int(r["episode"]), int(r["seat"])) for r in csv.DictReader((ROOT / "data/daily_perspectives.csv").open())}
by_date = {}
for r in csv.DictReader((ROOT / "data/perspectives_meta.csv").open()):
    if (int(r["episode"]), int(r["seat"])) in daily and r["episode_time"] and r["rating_before"]:
        d = date.fromisoformat(r["episode_time"][:10])
        by_date.setdefault(d, []).append(float(r["rating_before"]))
last = max(by_date)
reference = float(np.percentile(by_date[last], 90))
corpus, conditions, kept = [], [], 0
for r in rows:
    trace = ROOT / "data/traces" / f"{r['episode']}.txt"
    if not trace.exists():
        continue
    kept += 1
    split = {"validation": "validation", "test": "test"}.get(r["split"], "train")
    corpus.append(f"{r['episode']} {r['seat']} {r['team_id']} {r['submission']} {split} data/traces/{r['episode']}.txt\n")
    day = (date.fromisoformat(r["created"][:10]) - date(2026, 8, 15)).days
    conditions.append((r["episode"], r["seat"], f"{float(r['submission_score']) - reference:.1f}", day))
(ROOT / f"data/corpus_{name}.txt").write_text("".join(corpus))
with (ROOT / f"data/conditions_{name}.csv").open("w", newline="") as f:
    w = csv.writer(f); w.writerow(["episode", "seat", "rating", "day_index"]); w.writerows(conditions)
days = [c[3] for c in conditions]
print(f"{kept} of {len(rows)} perspectives exported; reference {reference:.0f} ({last}); days {min(days)}..{max(days)}")
