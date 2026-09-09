"""Separate changed contracts and improved witnesses in exposed warm reruns."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

from import_problems import physical_key
from baseline_study import cost


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(exist_ok=False)
    original = EXP / "data/warm_courses_v1/wheat12_d15_early_inputs_3s"
    records = []
    for name in ["warm_wheat_recompile_v1", "warm_wheat_resume_control_v1"]:
        current = EXP / "runs" / name / "course"
        for day in range(15, 30):
            a = json.loads((original / "days" / str(day) / "problem.json").read_text())
            b = json.loads((current / "days" / str(day) / "problem.json").read_text())
            hours = 23 if day == 29 else 24
            hires_a = {(e["hour"], e["order_index"]) for e in a["buy_schedule"] if e["op"] == "hire"}
            hires_b = {(e["hour"], e["order_index"]) for e in b["buy_schedule"] if e["op"] == "hire"}
            records.append({"run": name, "day": day, "same_physical_contract": physical_key(a, hours) == physical_key(b, hours),
                "new_hires_subset_of_old_slots": hires_b <= hires_a, "old_workers": a["worker_count"], "new_workers": b["worker_count"],
                "old_bill": float(cost([a["worker_count"]])[0]), "new_bill": float(cost([b["worker_count"]])[0]),
                "old_hire_slots": sorted(hires_a), "new_hire_slots": sorted(hires_b)})
    parent = sum(int(r["hire_cost"]) for r in csv.DictReader((EXP / "runs/warm_source_v1/DAYS.csv").open()) if int(r["day"]) >= 15)
    summary = []
    for name in sorted({r["run"] for r in records}):
        rows = [r for r in records if r["run"] == name]
        summary.append({"run": name, "same_contract_days": sum(r["same_physical_contract"] for r in rows),
            "new_hire_slots_subset_days": sum(r["new_hires_subset_of_old_slots"] for r in rows),
            "old_marginal_hire_bill": sum(r["old_bill"] for r in rows) - parent,
            "new_marginal_hire_bill": sum(r["new_bill"] for r in rows) - parent,
            "saved_hire_cost": sum(r["old_bill"] - r["new_bill"] for r in rows)})
    dependencies = json.loads((REPO / "experiments/v6/sep07_compositions_v0/final/evidence/EXTERNAL_DEPENDENCIES.json").read_text())
    selected = [r for r in dependencies["files"] if r["path"].startswith(("day_solver/", "fast_game_engine/"))]
    changed = [r["path"] for r in selected if not (REPO / r["path"]).is_file() or hashlib.sha256((REPO / r["path"]).read_bytes()).hexdigest() != r["sha256"]]
    report = {"scope": "Exposed reference-quality audit, not an estimator improvement. Fresh-start and archived-prefix invocations are separate controls. Compiler fixed repair is multithreaded and bounded outcomes need not reproduce.",
        "dependency_check": {"meaning": dependencies["meaning"], "engine_and_solver_files_checked": len(selected), "changed_since_archive_seal": changed},
        "summary": summary, "days": records}
    (args.output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"summary": summary, "dependency_check": report["dependency_check"]}, indent=2))


if __name__ == "__main__":
    main()
