"""Expert-iteration corpus: our seat of search-improved games (tools/search_games with
SEARCH_TRACES=<dir>) as extra BC labels.

Each trace <seed>_<seat>.txt of the k-th trace directory becomes one corpus line with a synthetic
episode id (3_000_000_000 + 100_000 * k + 2 * seed + seat), team id SEARCH_TEAM (style index 30 in
data/styles.json: "search-improved"), split by the usual episode rule, plus a
conditions row (strength --strength, day index --day) appended to the output
conditions file. Traces are verified by the extractor (load_replay parity).
usage: build_search_corpus.py out_corpus.txt out_conditions.csv trace_dir [trace_dir ...]
"""
import argparse
import csv
import json
from pathlib import Path

from build_corpus_v5 import split_of

ROOT = Path(__file__).resolve().parents[1]
SEARCH_TEAM = 3_000_000_030


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out_corpus")
    parser.add_argument("out_conditions")
    parser.add_argument("trace_dirs", nargs="+")
    parser.add_argument("--strength", type=float, default=150)
    parser.add_argument("--day", type=int, default=41)
    args = parser.parse_args()
    styles = json.loads((ROOT / "data/styles.json").read_text())
    if styles["styles"].get(str(SEARCH_TEAM)) != 30:
        styles["styles"][str(SEARCH_TEAM)] = 30
        styles["names"][str(SEARCH_TEAM)] = "search-improved (expert iteration)"
        (ROOT / "data/styles.json").write_text(json.dumps(styles, indent=1))
    lines, rows = [], []
    for k, trace_dir in enumerate(args.trace_dirs):
        for trace in sorted(Path(trace_dir).glob("*.txt")):
            seed, seat = map(int, trace.stem.split("_"))
            episode = 3_000_000_000 + 100_000 * k + 2 * seed + seat
            lines.append(f"{episode} {seat} {SEARCH_TEAM} 0 {split_of(episode)} {trace}\n")
            rows.append([episode, seat, f"{args.strength:.1f}", args.day])
    Path(args.out_corpus).write_text("".join(lines))
    with open(args.out_conditions, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["episode", "seat", "rating", "day_index"])
        writer.writerows(rows)
    print(len(lines), "search perspectives")


if __name__ == "__main__":
    main()
