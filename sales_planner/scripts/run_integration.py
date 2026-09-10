import argparse
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--candidate", default="compact_sales")
    parser.add_argument("--baseline", default="parent")
    parser.add_argument("--games", type=int, default=128)
    parser.add_argument("--seed-start", default="2026090900")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    jobs = []
    for native in (False, True):
        for opponent in ("pass", "nanare", "ahmed_v25"):
            for agent in (args.baseline, args.candidate):
                jobs.append((agent, opponent, native, args.games, 4, f"{'native' if native else 'independent'}_{agent}_{opponent}"))
    jobs.extend([
        (args.candidate, args.candidate, True, 4, 4, "self"),
        (args.candidate, args.baseline, True, 4, 1, "determinism_serial"),
        (args.candidate, args.baseline, True, 4, 4, "determinism_parallel"),
    ])

    def run(job):
        agent, opponent, native, games, threads, name = job
        output = args.output / f"{name}.json"
        command = [str(args.binary), "--a", agent, "--b", opponent, "--games", str(games),
                   "--seed-start", args.seed_start, "--seat-mode", "both", "--threads", str(threads),
                   "--budget-expansions", "100000", "--validate", "--output", str(output)]
        if native:
            command.append("--native-shops")
        with (args.output / f"{name}.log").open("w") as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        result = json.loads(output.read_text())
        print(json.dumps({"job": name, "games": len(result["games"]), "margin": result["mean_margin"],
                          "seconds": result["seconds"]}), flush=True)
        return {"name": name, "command": command}

    with ThreadPoolExecutor(max_workers=2) as pool:
        completed = list(pool.map(run, jobs))
    (args.output / "COMMANDS.json").write_text(json.dumps(completed, indent=2) + "\n")
    serial = json.loads((args.output / "determinism_serial.json").read_text())["games"]
    parallel = json.loads((args.output / "determinism_parallel.json").read_text())["games"]
    assert serial == parallel
    rows = []
    for panel in ("independent", "native"):
        for opponent in ("pass", "nanare", "ahmed_v25"):
            control = json.loads((args.output / f"{panel}_{args.baseline}_{opponent}.json").read_text())
            candidate = json.loads((args.output / f"{panel}_{args.candidate}_{opponent}.json").read_text())
            assert [(r["seed"], r["seat"]) for r in control["games"]] == [(r["seed"], r["seat"]) for r in candidate["games"]]
            rows.append({"panel": panel, "opponent": opponent,
                         "margin_gain": candidate["mean_margin"] - control["mean_margin"],
                         "cash_gain": candidate["mean_cash"] - control["mean_cash"],
                         "utility_gain": candidate["win_utility"] - control["win_utility"],
                         "tail_gain": candidate["margin_cvar10"] - control["margin_cvar10"],
                         "changed_action_games": sum(a["action_hash"] != b["action_hash"] for a, b in zip(control["games"], candidate["games"])),
                         "more_fault_games": sum(b["unit_faults"] > a["unit_faults"] for a, b in zip(control["games"], candidate["games"])),
                         "production_changed_games": sum(a["produced"] != b["produced"] for a, b in zip(control["games"], candidate["games"])),
                         "labor_changed_games": sum(a["worker_days"] != b["worker_days"] for a, b in zip(control["games"], candidate["games"]))})
    report = {"determinism": "identical complete game records across1/4threads", "comparisons": rows}
    (args.output / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2), flush=True)


if __name__ == "__main__":
    main()
