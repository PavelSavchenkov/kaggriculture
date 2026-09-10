"""Run each recorded player plan independently and retain candidate failures."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
FLAGS = {"wait_output_recent": "--outputs", "wait_history_recent": "--history-recent", "wait_history": "--history"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--binary", type=Path, required=True)
    args = parser.parse_args()
    binary = args.binary.resolve()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    jobs = []
    for variant in protocol["variants"]:
        folder = args.directory / variant
        folder.mkdir(exist_ok=True)
        for source, paths in [("old", protocol["old_cases"]), ("fresh", protocol["fresh_discovery_cases"])]:
            for path in paths:
                episode = int(Path(path).stem)
                for seat in range(2):
                    prefix = folder / f"{episode}_{seat}"
                    command = [str(binary), FLAGS[variant], "--source-actions", f"--seat{seat}", str(EXP / path)]
                    jobs.append({"variant": variant, "source": source, "episode": episode, "seat": seat,
                                 "command": command, "prefix": str(prefix)})
    identity = {"binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(), "jobs": jobs}
    record = args.directory / "RUNNER.json"
    if record.exists():
        assert json.loads(record.read_text()) == identity, "runner changed during resume"
    else:
        record.write_text(json.dumps(identity, indent=2) + "\n")

    def run(job):
        prefix = job["prefix"]
        status = Path(prefix + ".status.json")
        if status.exists():
            return json.loads(status.read_text())
        with Path(prefix + ".jsonl").open("w") as output, Path(prefix + ".events.jsonl").open("w") as error:
            result = subprocess.run(job["command"], stdout=output, stderr=error)
        value = {k: job[k] for k in ["variant", "source", "episode", "seat"]} | {"returncode": result.returncode}
        status.write_text(json.dumps(value) + "\n")
        return value

    with ThreadPoolExecutor(max_workers=4) as executor:
        statuses = list(executor.map(run, jobs))
    (args.directory / "RUN_STATUS.json").write_text(json.dumps(statuses, indent=2) + "\n")
    print(json.dumps({"jobs": len(statuses), "completed": sum(s["returncode"] == 0 for s in statuses),
                      "failed": [s for s in statuses if s["returncode"]]}, indent=2))


if __name__ == "__main__":
    main()
