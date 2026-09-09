"""Freeze shop-transfer policies before extracting predeclared new worlds."""
import csv
import hashlib
import json
import os
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    checks = EXP / "runs/warm_shop_adapter_check_v1/CHECK.json"
    assert json.loads(checks.read_text())["status"] == "passed"
    prior_seeds = {1201301738}
    for name in ["warm_seed_panel_v2", "warm_stage_seed_panel_v3"]:
        prior_seeds.update(json.loads((EXP / "data" / name / "PROTOCOL.json").read_text())["seeds"])
    worlds = []
    for index in range(12):
        seed = int(digest(f"warm_shop_panel_v1:seed:{index}")[:8], 16)
        if index < 8:
            tail = [int(digest(f"warm_shop_panel_v1:tail:{index}:{i}")[:2], 16) % 8 for i in range(5)]
            tail[0] = 6 + index % 2
            shops = [5, 2, 5, *tail]; kind = "changed_from_fourth_shop"
        else:
            shops = sorted(range(8), key=lambda shop: digest(f"warm_shop_panel_v1:permutation:{index}:{shop}"))
            kind = "full_permutation"
        worlds.append({"world": f"w{index:02}", "seed": seed, "shops": "".join(map(str, shops)), "shop_kind": kind})
    assert len({r["seed"] for r in worlds}) == len({r["shops"] for r in worlds}) == 12
    assert not {r["seed"] for r in worlds} & prior_seeds
    assert "52555401" not in {r["shops"] for r in worlds}
    specs = sorted((EXP / "data/warm_courses_v1/specs").glob("*.txt")); assert len(specs) == 7
    paths = [Path(__file__), checks, EXP / "scripts/run_warm_shop_compile.py", EXP / "scripts/benchmark_warm_shops.py",
             EXP / "scripts/analyze_warm_shops.py", EXP / "runs/warm_shop_report_check_v1/CHECK.json", EXP / "docs/warm_shop_test_v1.md", EXP / "runs/warm_shop_source_v1/SOURCE.patch",
             *sorted((EXP / "include").glob("*.hpp")), *specs,
             EXP / "models/context_candidate_dev_v2/context_model.hpp", EXP / "models/search_candidate_dev_v2/search_model.hpp"]
    for name in ["warm_shop_season", "verify_warm_shop_course", "warm_shop_compile_original", "warm_shop_compile_guided", "warm_shop_compile_stage"]:
        paths += [EXP / "build" / name, EXP / "source" / (name + ".cpp")]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "extracting", "worlds": worlds,
                "scope": "New environment seeds and shop sequences with the same exposed source/rival agents and seven specifications. Eight worlds change demand from the fourth shop; four have all-shop permutations. These are designed transfer cases, not a random estimate over all games.",
                "selection": "Exactly these twelve SHA-derived worlds and seven existing specs. Filter only unchanged compiler construction preconditions: 75 owned tiles, first expansion cells locked and a strict first-day source witness. No replacement for exclusions and no model/outcome-based eligibility.",
                "primary_candidate": "unchanged .02 whole-call deferral", "secondary_candidate": "unchanged .10 after-repair cold-stage deferral", "control": "original compiler with the same fixture adapter",
                "acceptance": "At least 24 eligible paired cases across at least six worlds; at least 20 percent mean complete CPU saving, positive world-cluster 95 percent saved-CPU interval, no lost certificates, no higher mean common-success hire bill and equal production on common successes. Retain every failed/native-crashed child and its complete CPU. Stage is secondary and is not selected based on partial results.",
                "sha256": {str(p.relative_to(EXP)): sha(p) for p in paths}}
    output = EXP / "data/warm_shop_panel_v1"; output.mkdir(exist_ok=False)
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    snapshot = EXP / "snapshots/warm_shops_v1"; snapshot.mkdir(exist_ok=False)
    for path in paths:
        target = snapshot / path.relative_to(EXP); target.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(path, target)
    (snapshot / "FREEZE.json").write_text(json.dumps(protocol, indent=2) + "\n")

    def extract(world):
        environment = dict(os.environ); environment["LABOR_WARM_SHOPS"] = world["shops"]
        with (output / (world["world"] + ".log")).open("w") as stream:
            subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                            str(EXP / "build/warm_shop_season"), str(output / world["world"]), str(world["seed"])],
                           cwd=EXP, env=environment, stdout=stream, stderr=subprocess.STDOUT, check=True)

    with ThreadPoolExecutor(max_workers=2) as pool: list(pool.map(extract, worlds))
    cases, excluded = [], []
    for world in worlds:
        folder = output / world["world"]
        days = {int(r["day"]): r for r in csv.DictReader((folder / "DAYS.csv").open())}
        for spec in specs:
            rotations = [tuple(map(int, line.split())) for line in spec.read_text().splitlines()]
            first = min(r[1] for r in rotations); path = folder / str(first) / "problem.json"
            problem = json.loads(path.read_text()); tiles = problem["start"]["managed_tiles"]
            owned = sum(t["state"]["kind"] != "locked" for t in tiles)
            reasons = []
            if owned != 75: reasons.append(f"source_owns_{owned}_tiles_instead_of_75")
            if any(tiles[cell]["state"]["kind"] != "locked" for cell, day, _ in rotations if day == first): reasons.append("expansion_cells_not_locked")
            if days[first]["strict"] != "1": reasons.append("source_witness_not_strict")
            row = {**world, "id": world["world"] + "_" + spec.stem, "spec": str(spec.relative_to(EXP)), "first_day": first,
                   "first_problem_sha256": sha(path), "source_panel": str(folder.relative_to(EXP))}
            if reasons: excluded.append({**row, "reasons": reasons})
            else: cases.append(row)
    (output / "CASES.json").write_text(json.dumps(cases, indent=2) + "\n")
    (output / "EXCLUDED.json").write_text(json.dumps(excluded, indent=2) + "\n")
    protocol.update(status="prepared", completed_utc=datetime.now(timezone.utc).isoformat(), eligible=len(cases), excluded=len(excluded))
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps({"worlds": worlds, "eligible": len(cases), "excluded": excluded}, indent=2))


if __name__ == "__main__":
    main()
