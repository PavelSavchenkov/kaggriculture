"""Make a content-keyed feature manifest from an experiment-local index."""
import argparse
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("index", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    rows = [json.loads(line) for line in args.index.read_text().splitlines()]
    unique = {row["source_sha256"]: row for row in rows}
    with args.output.open("x") as output:
        for sha, row in sorted(unique.items()):
            output.write(f"{sha[:20]} {EXP / row['problem']}\n")
    print(f"{len(unique)} unique inputs from {len(rows)} records")


if __name__ == "__main__":
    main()
