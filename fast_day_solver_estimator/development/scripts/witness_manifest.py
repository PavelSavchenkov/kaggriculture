"""Index witness/problem pairs without claiming their validity."""
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
    unique = {}
    for row in rows:
        for witness in row["witnesses"]:
            key = row["source_sha256"][:20] + "_" + witness["sha256"][:20]
            unique[key] = (row, witness)
    with args.output.open("x") as output:
        for key, (row, witness) in sorted(unique.items()):
            output.write(f"{key} {EXP / row['problem']} {EXP / witness['path']}\n")
    print(f"{len(unique)} unique witness/problem pairs")


if __name__ == "__main__":
    main()
