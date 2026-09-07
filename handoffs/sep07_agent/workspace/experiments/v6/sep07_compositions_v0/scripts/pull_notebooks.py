"""Retrieve public notebook sources for inspection; never execute notebook cells."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


EXPERIMENT = Path(__file__).resolve().parents[1]
RESEARCH = EXPERIMENT / "research"
REFS = [
    "yhay81/six-day-public-state-fieldbook",
    "yhay81/three-day-shop-router",
    "tetsutani/shape-the-shop-work-the-pasture-kaggriculture",
    "thomastschinkel/kaggriculture-public-state-router-74-5-win-rate",
    "dmitriigluzdov/kaggriculture-goose-portfolio-historical-lb-2615",
    "destbreso/v7-38-finance7-a-full-agent-layer-by-layer",
    "destbreso/97-predictable-extracting-decision-trees",
    "junaid512/02-adaptive-replay-agent",
    "lynnsakurai/farming-score-a-mathematical-approach",
    "boatlee/v29-r1-adaptive-market-hysteresis",
    "kaitofukami/238-238-known-streams-v58-minimax-closed-loop",
    "busyaprime/kaggriculture-800-paired-games-and-the-map-wins",
]


def pull(ref):
    folder = RESEARCH / "notebooks" / ref
    folder.mkdir(parents=True, exist_ok=True)
    command = ["conda", "run", "-n", "kaggriculture", "kaggle", "kernels", "pull", ref, "-p", str(folder), "-m"]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    (folder / "pull.log").write_text(result.stdout + result.stderr)
    record = {"ref": ref, "url": f"https://www.kaggle.com/code/{ref}", "returncode": result.returncode, "files": {}}
    if result.returncode:
        print(f"failed {ref}: {result.returncode}", flush=True)
        return record
    for path in folder.iterdir():
        if path.suffix in {".ipynb", ".py", ".json"}:
            record["files"][path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
        if path.suffix == ".ipynb":
            notebook = json.loads(path.read_text())
            markdown, code = [], []
            for cell in notebook["cells"]:
                target = markdown if cell["cell_type"] == "markdown" else code
                target.append("".join(cell["source"]))
            (folder / "notes.md").write_text("\n\n".join(markdown))
            (folder / "cells.py.txt").write_text("\n\n# ---- cell ----\n\n".join(code))
    print(f"pulled {ref}", flush=True)
    return record


def main():
    global RESEARCH
    parser=argparse.ArgumentParser()
    parser.add_argument("--research-dir",type=Path,default=RESEARCH)
    parser.add_argument("--refs",nargs="+",default=REFS)
    args=parser.parse_args();RESEARCH=args.research_dir.resolve()
    assert RESEARCH.is_relative_to(EXPERIMENT)
    with ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(pull, ref) for ref in args.refs]
        records = [future.result() for future in as_completed(futures)]
    (RESEARCH / "notebook_manifest.json").write_text(json.dumps(records, indent=2))


if __name__ == "__main__":
    main()
