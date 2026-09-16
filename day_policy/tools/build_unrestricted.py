"""Build unrestricted development and validation cases from packaged replays."""
import argparse
import csv
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import subprocess


PACKAGE = Path(__file__).resolve().parents[1]


def build_cohort(name, labels, cohorts, traces, output, build):
    games = [game for label in labels for game in cohorts[label]["games"]]
    cohort = output / f"{name}.txt"
    cohort.write_text("".join(
        f"{game['game']} {game['seat']} {game['rank']} {traces}/{game['game']}.txt\n"
        for game in games
    ))
    binary = output / f"{name}.bin"
    exclusions = output / f"{name}.csv"
    with (output / f"{name}.log").open("w") as log:
        subprocess.run(
            [build / "contract_extract", cohort, binary, exclusions],
            stdout=log,
            stderr=subprocess.STDOUT,
            check=True,
        )
    rows = list(csv.DictReader(exclusions.open()))
    eligible = [row for row in rows if row["reason"] == "eligible"]
    feature = lambda key: sum(int(row[key]) > 0 for row in eligible)
    return {
        "games": len(games),
        "days": len(rows),
        "eligible": len(eligible),
        "reasons": dict(Counter(row["reason"] for row in rows)),
        "eligible_features": {
            "late_inputs": feature("late_inputs"),
            "fertilizer_buys": feature("fertilizer_buys"),
            "fertilizer_pickups": feature("fertilizer_pickups"),
            "late_hires": feature("late_hires"),
            "any_removed_restriction": sum(any(int(row[key]) > 0 for key in (
                "late_inputs", "fertilizer_buys", "fertilizer_pickups", "late_hires"
            )) for row in eligible),
        },
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True)
    traces = args.work / "traces"
    traces.mkdir()
    cohorts = json.loads((PACKAGE / "data/COHORTS.json").read_text())
    manifest = json.loads((PACKAGE / "data/TRACES.json").read_text())
    game_ids = {str(game["game"]) for cohort in cohorts.values() for game in cohort["games"]}
    for game in sorted(game_ids):
        content = gzip.decompress((PACKAGE / f"data/replays/{game}.txt.gz").read_bytes())
        if len(content) != manifest[game]["bytes"] or hashlib.sha256(content).hexdigest() != manifest[game]["sha256"]:
            raise ValueError(f"Trace checksum mismatch: {game}")
        (traces / f"{game}.txt").write_bytes(content)
    report = {
        "definition": "Exact purchase hours; fertilizer purchases/pickups and hires at any hour are supported.",
        "dev": build_cohort("dev", ["dev"], cohorts, traces, args.work, args.build),
        "valid": build_cohort("valid", ["639", "645"], cohorts, traces, args.work, args.build),
    }
    (args.work / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
