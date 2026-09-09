"""Predeclare new warm worlds and filter only unsupported course preconditions."""
import csv
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    output = EXP / "data/warm_seed_panel_v2"; output.mkdir(exist_ok=False)
    seeds = [int(hashlib.sha256(f"warm_seed_panel_v2:{i}".encode()).hexdigest()[:8], 16) for i in range(8)]
    assert len(set(seeds)) == 8 and 1201301738 not in seeds
    specs = sorted((EXP / "data/warm_courses_v1/specs").glob("*.txt")); assert len(specs) == 7
    paths = [Path(__file__), EXP / "build/warm_season", EXP / "build/warm_compile_early", EXP / "build/warm_compile_guided",
             EXP / "models/context_candidate_dev_v2/context_model.hpp", *specs]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "extracting", "seeds": seeds,
        "scope": "New environment seeds for the same exposed source/rival and fixed shop sequence; not new agent families or shop generalization.",
        "selection": "Exactly eight SHA256-derived uint32 seeds, indices zero through seven. Do not replace excluded or difficult seeds.",
        "eligibility": "At the prescribed first expansion day the source owns exactly three quadrants, the prescribed expansion cells are locked, and the source physical witness is strict. These are unchanged compiler construction preconditions. Do not inspect candidate outcomes or model forecasts to select cases.",
        "comparison": "Freeze the unchanged original and probability-0.02 guided compiler binaries now. All eligible seed/course pairs will receive both methods with deterministic balanced order. Retain all construction and solver failures. No resume or model refit.",
        "sha256": {str(p.relative_to(EXP)): sha(p) for p in paths}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")

    def extract(seed):
        path = output / str(seed)
        command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                   str(EXP / "build/warm_season"), str(path), str(seed)]
        with (output / f"{seed}.log").open("w") as stream:
            subprocess.run(command, cwd=EXP, stdout=stream, stderr=subprocess.STDOUT, check=True)
        return seed

    with ThreadPoolExecutor(max_workers=2) as pool: list(pool.map(extract, seeds))
    cases, excluded = [], []
    for seed in seeds:
        days = {int(r["day"]): r for r in csv.DictReader((output / str(seed) / "DAYS.csv").open())}
        for spec in specs:
            rotations = [tuple(map(int, line.split())) for line in spec.read_text().splitlines()]
            first = min(r[1] for r in rotations)
            path = output / str(seed) / str(first) / "problem.json"
            p = json.loads(path.read_text()); tiles = p["start"]["managed_tiles"]
            owned = sum(t["state"]["kind"] != "locked" for t in tiles)
            reasons = []
            if owned != 75: reasons.append(f"source_owns_{owned}_tiles_instead_of_75")
            if any(tiles[cell]["state"]["kind"] != "locked" for cell, day, _ in rotations if day == first): reasons.append("expansion_cells_not_locked")
            if days[first]["strict"] != "1": reasons.append("source_witness_not_strict")
            row = {"id": f"s{seed}_{spec.stem}", "seed": seed, "spec": str(spec.relative_to(EXP)), "first_day": first,
                   "first_problem_sha256": sha(path), "source_panel": str((output / str(seed)).relative_to(EXP))}
            if reasons: excluded.append({**row, "reasons": reasons})
            else: cases.append(row)
    (output / "CASES.json").write_text(json.dumps(cases, indent=2) + "\n")
    (output / "EXCLUDED.json").write_text(json.dumps(excluded, indent=2) + "\n")
    protocol.update(status="prepared", completed_utc=datetime.now(timezone.utc).isoformat(), eligible=len(cases), excluded=len(excluded))
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps({"seeds": seeds, "eligible": len(cases), "excluded": excluded}, indent=2))


if __name__ == "__main__":
    main()
