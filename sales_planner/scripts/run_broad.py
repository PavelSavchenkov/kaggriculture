import argparse
import json
import math
import statistics
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


def utility(row):
    return float(row["cash"] > row["opponent_cash"]) + 0.5 * (row["cash"] == row["opponent_cash"])


def margin(row):
    return row["cash"] - row["opponent_cash"]


def tail(rows):
    return statistics.mean(sorted(map(margin, rows))[:math.ceil(len(rows) / 10)])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    identity = {"binary": str(args.binary.resolve()), "protocol": protocol}
    identity_path = args.directory / "RUNNER.json"
    if identity_path.exists():
        assert json.loads(identity_path.read_text()) == identity
    else:
        identity_path.write_text(json.dumps(identity, indent=2) + "\n")
    jobs = [(panel, opponent, agent) for panel in protocol["shop_panels"]
            for opponent in protocol["opponents"]
            for agent in (protocol["baseline"], protocol["candidate"])]

    def run(job):
        panel, opponent, agent = job
        name = f"{panel}_{agent}_{opponent}"
        output = args.directory / f"{name}.json"
        command = [str(args.binary), "--a", agent, "--b", opponent,
                   "--games", str(protocol["seeds_per_opponent_panel"]),
                   "--seed-start", str(protocol["seed_start"]), "--seat-mode", "both",
                   "--threads", str(protocol["threads_per_batch"]),
                   "--budget-expansions", str(protocol["max_expansions"]),
                   "--validate", "--output", str(output)]
        if panel == "native":
            command.append("--native-shops")
        if not output.exists():
            with (args.directory / f"{name}.log").open("w") as log:
                subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        rows = json.loads(output.read_text())["games"]
        assert len(rows) == 2 * protocol["seeds_per_opponent_panel"]
        print(json.dumps({"job": name, "games": len(rows), "mean_margin": statistics.mean(map(margin, rows))}), flush=True)
        return {"job": name, "command": command}

    started = time.perf_counter()
    with ThreadPoolExecutor(max_workers=protocol["parallel_batches"]) as pool:
        commands = list(pool.map(run, jobs))
    (args.directory / "COMMANDS.json").write_text(json.dumps(commands, indent=2) + "\n")
    reports = []
    for panel in protocol["shop_panels"]:
        for opponent in protocol["opponents"]:
            read = lambda agent: json.loads((args.directory / f"{panel}_{agent}_{opponent}.json").read_text())["games"]
            baseline = read(protocol["baseline"])
            candidate = read(protocol["candidate"])
            assert [(r["seed"], r["seat"]) for r in baseline] == [(r["seed"], r["seat"]) for r in candidate]
            deltas = [margin(b) - margin(a) for a, b in zip(baseline, candidate)]
            reports.append({"panel": panel, "opponent": opponent, "games": len(deltas),
                            "margin_gain": statistics.mean(deltas), "worst_gain": min(deltas),
                            "positive": sum(d > 0 for d in deltas), "negative": sum(d < 0 for d in deltas),
                            "utility_gain": statistics.mean(utility(b) - utility(a) for a, b in zip(baseline, candidate)),
                            "tail_gain": tail(candidate) - tail(baseline),
                            "cash_gain": statistics.mean(b["cash"] - a["cash"] for a, b in zip(baseline, candidate)),
                            "production_changes": sum(a["produced"] != b["produced"] for a, b in zip(baseline, candidate)),
                            "labor_changes": sum(a["worker_days"] != b["worker_days"] for a, b in zip(baseline, candidate)),
                            "extra_fault_cases": sum(b["unit_faults"] > a["unit_faults"] or b["opponent_unit_faults"] > a["opponent_unit_faults"]
                                                     for a, b in zip(baseline, candidate))})
    result = {"comparisons": reports, "wall_seconds": time.perf_counter() - started,
              "scope": protocol["role"]}
    (args.directory / "SUMMARY.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2), flush=True)


if __name__ == "__main__":
    main()
