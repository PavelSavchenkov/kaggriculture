"""Compare necessary public starting-state features before treating tapes as forecasts."""
import argparse
import collections
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--protocol", type=Path, required=True)
    parser.add_argument("--extended", action="store_true")
    args = parser.parse_args()
    source_protocol = json.loads(args.protocol.read_text())
    protocol = {key: source_protocol[key] for key in
                ["opponents", "seed_start", "seeds_per_opponent", "seat_mode", "shop_panel", "scenario_cases"]}
    args.directory.mkdir(parents=True, exist_ok=False)
    protocol.update(
        created_utc=datetime.now(timezone.utc).isoformat(),
        binary=str(args.binary),
        binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        step=226,
        purpose="Necessary public-state compatibility checks; matching does not establish full compatibility.",
        extended_public_fields=args.extended,
    )
    (args.directory / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    rows = []
    commands = []
    for opponent in protocol["opponents"]:
        output = args.directory / f"{opponent}.jsonl"
        command = [str(args.binary), opponent, str(protocol["seed_start"]),
                   str(protocol["seeds_per_opponent"]), str(output), *protocol["scenario_cases"]]
        commands.append(command)
        (args.directory / "COMMANDS.json").write_text(json.dumps(commands, indent=2) + "\n")
        subprocess.run(command, check=True)
        rows.extend(json.loads(line) for line in output.read_text().splitlines())
    features = ["counts_equal", "placement_equal", "workers_equal", "positions_equal", "land_equal"]
    if args.extended:
        features += ["tile_state_equal", "worker_order_equal", "hiring_equal"]
    report = {}
    for opponent in ["all", *protocol["opponents"]]:
        selected = [r for r in rows if opponent == "all" or r["opponent"] == opponent]
        decisions = collections.defaultdict(list)
        for row in selected:
            decisions[(row["opponent"], row["seed"], row["seat"])].append(row)
        report[opponent] = {
            "source_current_pairs": len(selected),
            "decisions": len(decisions),
            "matching_pairs": {f: sum(r[f] for r in selected) for f in features},
            "all_features_matching_pairs": sum(all(r[f] for f in features) for r in selected),
            "decisions_with_any_matching_source": {
                f: sum(any(r[f] for r in group) for group in decisions.values()) for f in features
            },
            "decisions_with_any_all_features_matching_source": sum(
                any(all(r[f] for f in features) for r in group) for group in decisions.values()
            ),
        }
        if args.extended:
            report[opponent]["cash_matching_pairs"] = sum(r["cash_equal"] for r in selected)
            report[opponent]["all_public_fields_including_cash_matching_pairs"] = sum(
                r["cash_equal"] and all(r[f] for f in features) for r in selected)
            report[opponent]["decisions_with_any_complete_public_match"] = sum(
                any(r["cash_equal"] and all(r[f] for f in features) for r in group)
                for group in decisions.values())
    expected = len(protocol["opponents"]) * protocol["seeds_per_opponent"] * 2 * len(protocol["scenario_cases"]) * 2
    assert len(rows) == expected
    (args.directory / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    examples = []
    for opponent in protocol["opponents"]:
        examples.append(next(r for r in rows if r["opponent"] == opponent))
    (args.directory / "EXAMPLES.json").write_text(json.dumps(examples, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
