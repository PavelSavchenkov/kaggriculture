"""Run and archive the C++ estimate/compile/exact loop; no Python strategy logic."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("program", type=int)
    parser.add_argument("opponent", choices=["public_router", "opening_router_v4", "junghoon_78", "binghua_116", "justin_150", "justin_recall_v0", "justin_guarded_hires_001_best", "king_rc4"])
    parser.add_argument("run", type=Path)
    parser.add_argument("--baseline-games", type=int, default=8)
    parser.add_argument("--games", type=int, default=16)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--terminal", action="store_true")
    args = parser.parse_args()
    run = args.run.resolve()
    assert run.is_relative_to(EXP / "runs")
    if run.exists():
        raise FileExistsError(run)
    output = subprocess.check_output(["conda", "run", "-n", "kaggriculture", "python", str(EXP / "scripts/build_ticket_search.py")], cwd=ROOT, text=True)
    binary = Path(output.strip().splitlines()[-1])
    assert binary.is_file() and binary.is_relative_to(EXP / "build")
    command = ["conda", "run", "-n", "kaggriculture", str(binary), str(args.program), str(run),
               "--b", args.opponent, "--baseline-games", str(args.baseline_games), "--games", str(args.games),
               "--seed-start", "1000", "--seat-mode", "both", "--threads", str(args.threads), "--validate"]
    if args.terminal:
        command.append("--terminal")
    result = subprocess.run(command, cwd=ROOT)
    record = {"command": command, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
              "build": json.loads((binary.parent / "build.json").read_text()), "returncode": result.returncode,
              "scope": "Development seeds1000+, both seats; C++ owns tracing, estimation, selection, compilation and exact evaluation. No automatic promotion."}
    if run.is_dir():
        (run / "RUN.json").write_text(json.dumps(record, indent=2) + "\n")
    result.check_returncode()
    for script in ["complete_ticket_lineage.py", "calibrate_ticket_estimates.py"]:
        subprocess.run(["conda", "run", "-n", "kaggriculture", "python", str(EXP / "scripts" / script), str(run)], check=True, cwd=ROOT)
    print(run)


if __name__ == "__main__":
    main()
