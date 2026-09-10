import argparse
import json
import subprocess
from pathlib import Path

from build import EXP, build


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    binary = build(EXP / "source/check_calendars.cpp")
    files = sorted(args.directory.glob("*.calendar"))
    assert files
    command = [str(binary), *map(str, files)]
    result = subprocess.run(command, capture_output=True, text=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    report = {"command": command, "exit_code": result.returncode,
              "cases": [json.loads(line) for line in result.stdout.splitlines()], "diagnostic": result.stderr}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))
    result.check_returncode()


if __name__ == "__main__":
    main()
