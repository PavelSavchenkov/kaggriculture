"""Run the exact two-stage training and native export for agent_sep23."""
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DATA_HASH = "0ef20bdc02c99f3c157ad823b878f9e021dfb1d02df2494f9a6f941efbd0e45d"
MODEL_HASH = "9c22aaa6051568a592afa39e710468f9f0a9e34edfede89271c6a207d5de47e8"


def run(stage, arguments):
    directory = ROOT / stage
    environment = os.environ.copy()
    environment["PYTHONPATH"] = str(directory)
    subprocess.run([sys.executable, "-m", "bc.train", *map(str, arguments)],
                   cwd=directory, env=environment, check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    data = args.data.resolve()
    output = args.output.resolve()
    summary = json.loads((data / "summary.json").read_text())
    if summary["data_hash"] != DATA_HASH:
        raise ValueError("Dataset identity does not match the selected corpus")
    output.mkdir(parents=True, exist_ok=False)
    stage1 = output / "stage1"
    common = ["--data", data, "--width", 128, "--depth", 2, "--batch", 512,
              "--lr", .001, "--weight-decay", .001, "--rank", 20, "--seed", 2301,
              "--financial", "--coordinated", "--accounting", "--space-mask", "--product-plans"]
    run("stage1", [*common, "--run", stage1, "--epochs", 82,
                   "--cpu-threads", 4, "--save-every", 20])
    stage2 = output / "stage2"
    run("stage2", [*common, "--run", stage2, "--epochs", 500,
                   "--cpu-threads", 2, "--save-every", 125, "--submission", 56424800,
                   "--compile-loss", "--initialize", stage1 / "best.pt"])
    environment = os.environ.copy()
    environment["PYTHONPATH"] = str(ROOT / "stage2")
    model = output / "model.bin"
    subprocess.run([sys.executable, "-m", "bc.export", stage2 / "best.pt", model],
                   cwd=ROOT / "stage2", env=environment, check=True)
    actual = hashlib.sha256(model.read_bytes()).hexdigest()
    if actual != MODEL_HASH:
        raise ValueError(f"Native model hash mismatch: {actual}")
    print(f"verified {model} {actual}")


if __name__ == "__main__":
    main()
