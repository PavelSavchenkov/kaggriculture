"""Run fixed C++ course comparisons in batches; preserve failed output."""
import argparse
import hashlib
import json
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    identity = {"binary": str(args.binary.resolve()),
                "sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(), "protocol": protocol}
    path = args.directory / "RUNNER.json"
    if path.exists():
        assert json.loads(path.read_text()) == identity
    else:
        path.write_text(json.dumps(identity, indent=2) + "\n")

    def run(job):
        panel, opponent = job
        name = panel + "_" + opponent
        status_path = args.directory / (name + ".status.json")
        if status_path.exists():
            return json.loads(status_path.read_text())
        output = args.directory / (name + ".jsonl")
        command = [str(args.binary), opponent, str(protocol["seed_start"]),
                   str(protocol["seeds_per_opponent_panel"]), str(output)]
        if panel == "native":
            command.append("--native-shops")
        if protocol.get("inventory_guard", False):
            command.append("--inventory-guard")
        if protocol.get("day_timing", False):
            command.append("--day-hold")
        if protocol.get("delivery_timing", False):
            command.append("--delivery-bound")
        if protocol.get("multiple_sales", False):
            command.append("--multiple-sales")
        if protocol.get("holding_exchange", False):
            command.append("--holding-exchange")
        started = time.perf_counter()
        with (args.directory / (name + ".log")).open("w") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        status = {"panel": panel, "opponent": opponent, "command": command,
                  "exit_code": result.returncode, "seconds": time.perf_counter() - started,
                  "output": output.name}
        status_path.write_text(json.dumps(status, indent=2) + "\n")
        print(json.dumps(status), flush=True)
        return status

    jobs = [(panel, opponent) for panel in protocol["shop_panels"] for opponent in protocol["opponents"]]
    with ThreadPoolExecutor(max_workers=protocol["parallel_batches"]) as pool:
        statuses = list(pool.map(run, jobs))
    (args.directory / "RUN_STATUS.json").write_text(json.dumps(statuses, indent=2) + "\n")


if __name__ == "__main__":
    main()
