"""Alternate preserved evaluator binaries on identical complete calendars."""
import argparse
import hashlib
import json
import statistics
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--label", required=True)
    parser.add_argument("--pairs", type=int, default=7)
    parser.add_argument("--repetitions", type=int, default=20)
    args = parser.parse_args()
    assert args.pairs > 0 and args.repetitions > 0 and args.label.isidentifier()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    record = args.directory / f"{args.label}_RUNS.json"
    assert not record.exists()
    rows = []
    for pair in range(args.pairs):
        for variant in (["control", "candidate"] if pair % 2 == 0 else ["candidate", "control"]):
            binary = protocol["control_binary" if variant == "control" else "candidate_benchmark_binary"]
            command = [binary, str(args.repetitions), *protocol["performance_cases"]]
            value = subprocess.run(command, capture_output=True, text=True, check=True)
            rows.append({"pair": pair, "variant": variant, "command": command,
                         "sha256": hashlib.sha256(Path(binary).read_bytes()).hexdigest(), **json.loads(value.stdout)})
            record.write_text(json.dumps(rows, indent=2) + "\n")
    assert len({x["checksum"] for x in rows}) == 1
    ratios = []
    for pair in range(args.pairs):
        values = {x["variant"]: x for x in rows if x["pair"] == pair}
        ratios.append(values["candidate"]["seconds"] / values["control"]["seconds"])
    report = {
        "paired_elapsed_ratios": ratios, "median_ratio": statistics.median(ratios),
        "control_median_microseconds": statistics.median(x["microseconds_per_call"] for x in rows if x["variant"] == "control"),
        "candidate_median_microseconds": statistics.median(x["microseconds_per_call"] for x in rows if x["variant"] == "candidate"),
        "calls_per_run": rows[0]["calls"],
        "scope": "Existing financial-evaluator client; loading, extraction and live-policy work excluded. Compiler optimization and timing noise apply.",
    }
    (args.directory / f"{args.label}.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
