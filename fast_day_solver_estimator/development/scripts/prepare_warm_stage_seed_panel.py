"""Freeze the stage candidate before opening a disjoint seed panel."""
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
    checks = EXP / "runs/warm_stage_controls_v1/PROTOCOL.json"
    assert json.loads(checks.read_text())["status"] == "completed", "finish development correctness controls first"
    output = EXP / "data/warm_stage_seed_panel_v3"; output.mkdir(exist_ok=False)
    seeds = [int(hashlib.sha256(f"warm_stage_seed_panel_v3:{i}".encode()).hexdigest()[:8], 16) for i in range(8)]
    old_seeds = json.loads((EXP / "data/warm_seed_panel_v2/PROTOCOL.json").read_text())["seeds"]
    assert len(set(seeds)) == 8 and not set(seeds) & {1201301738, *old_seeds}
    specs = sorted((EXP / "data/warm_courses_v1/specs").glob("*.txt")); assert len(specs) == 7
    paths = [Path(__file__), EXP / "build/warm_season", EXP / "build/warm_compile_early", EXP / "build/warm_compile_stage",
             EXP / "source/warm_compile_stage.cpp", EXP / "scripts/run_warm_stage.py", EXP / "scripts/benchmark_warm_stage_seeds.py",
             EXP / "scripts/analyze_warm_seeds.py", EXP / "models/context_candidate_dev_v2/context_model.hpp", checks,
             EXP / "snapshots/planning_wave_v3/FREEZE.json", *sorted((EXP / "include").glob("*.hpp")), *specs]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "extracting", "seeds": seeds,
        "scope": "New environment seeds for the same exposed source/rival and fixed shop sequence; not new agent families or shop generalization.",
        "selection": "Exactly eight SHA256-derived uint32 seeds, indices zero through seven. Do not replace excluded or difficult seeds.",
        "eligibility": "At the prescribed first expansion day the source owns exactly three quadrants, the prescribed expansion cells are locked, and the source physical witness is strict. These are unchanged compiler construction preconditions. Do not inspect candidate outcomes or model forecasts to select cases.",
        "comparison": "Freeze the unchanged original and probability-0.10 cold-stage compiler binaries now. Keep reuse and repair; predict only after repair fails. Retry deferred cold calls without repeating repair if other counts fail. All eligible pairs receive both methods in deterministic balanced order. Retain all failures; no resume, refit or threshold change.",
        "acceptance": "At least 20 percent mean complete-compiler CPU saving, positive seed-cluster saved-time interval, no lost certificates and no greater mean hire bill on common successes. Include every failure; report production equality and both seed/course uncertainty.",
        "sha256": {str(p.relative_to(EXP)): sha(p) for p in paths}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    snapshot = EXP / "snapshots/warm_stage_v1"; snapshot.mkdir(exist_ok=False)
    for path in paths:
        destination = snapshot / path.relative_to(EXP); destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(path.read_bytes())
        destination.chmod(path.stat().st_mode)
    (snapshot / "FREEZE.json").write_text(json.dumps(protocol, indent=2) + "\n")

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
