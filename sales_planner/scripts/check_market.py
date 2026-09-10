import argparse
import json
import subprocess
from pathlib import Path

from build import EXP, build


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--games", type=int, default=256)
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    binary = build(EXP / "source/check_market.cpp", debug=args.debug)
    command = [str(binary), str(args.games)]
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    report = {"command": command, "debug": args.debug, **json.loads(result.stdout)}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
