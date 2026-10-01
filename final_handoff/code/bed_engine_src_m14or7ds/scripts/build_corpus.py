"""Corpus of top-player perspectives with a whole-episode split (both seats together).

Sources: data/perspectives_sep23.json (existing traces) and data/fresh_selection.json
(fresh replays, once exported). Split by episode SHA256 "sep24-BC-opus": 80% train,
10% validation, 10% test. Writes data/corpus.txt: episode seat team_id submission split trace.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def split_of(episode):
    bucket = int.from_bytes(hashlib.sha256(f"sep24-BC-opus:{episode}".encode()).digest()[:8], "little") % 10
    return "test" if bucket == 0 else "validation" if bucket == 1 else "train"


rows = {}
for row in json.loads((ROOT / "data/perspectives_sep23.json").read_text()):
    rows[(row["episode"], row["seat"])] = (row["team_id"], row["submission"], row["team"], "sep23")
fresh = ROOT / "data/fresh_selection.json"
if fresh.exists():
    for row in json.loads(fresh.read_text())["rows"]:
        if (ROOT / "data/traces" / f"{row['episode']}.txt").exists():
            rows[(row["episode"], row["seat"])] = (row["team_id"], row["submission"], row["team"], "fresh")
lines, counts = [], {}
for (episode, seat), (team, submission, name, source) in sorted(rows.items()):
    split = split_of(episode)
    counts[(source, split)] = counts.get((source, split), 0) + 1
    lines.append(f"{episode} {seat} {team} {submission} {split} data/traces/{episode}.txt\n")
(ROOT / "data/corpus.txt").write_text("".join(lines))
print(len(lines), "perspectives;", sorted(counts.items()))
