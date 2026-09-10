import argparse
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    flags = {"control": [], "own_repair": ["--repair-own"], "both_repair": ["--repair-both"]}
    flags.update(protocol.get("variant_flags", {}))
    for variant in protocol["variants"]:
        directory = args.directory / variant
        directory.mkdir(exist_ok=False)
        (directory / "PROTOCOL.json").write_text(json.dumps({**protocol, "variant": variant}, indent=2) + "\n")

    def run(job):
        variant, opponent = job
        directory = args.directory / variant
        command = [protocol["binary"], opponent, str(protocol["seed_start"]), str(protocol["seeds_per_opponent"]),
                   str(directory / f"{opponent}.jsonl"), *protocol["scenario_cases"], *flags[variant]]
        (directory / f"{opponent}_COMMAND.json").write_text(json.dumps(command, indent=2) + "\n")
        with (directory / f"{opponent}.log").open("w") as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        print(variant, opponent, "complete", flush=True)

    jobs = [(variant, opponent) for variant in protocol["variants"] for opponent in protocol["opponents"]]
    with ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(run, jobs))


if __name__ == "__main__":
    main()
