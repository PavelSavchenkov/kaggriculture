"""Join panel membership for one shared physical reference sweep."""
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("inputs", type=Path, nargs="+")
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    rows = [json.loads(line) for path in args.inputs for line in path.read_text().splitlines()]
    (args.output / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    report = {"utc": datetime.now(timezone.utc).isoformat(), "records": len(rows),
              "physical_contracts": len({r["physical_key"] for r in rows}),
              "input_variants": len({r["source_sha256"] for r in rows}),
              "purpose": "Share identical physical reference calls across panels; preserve all panel membership.",
              "inputs": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in args.inputs}}
    (args.output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
