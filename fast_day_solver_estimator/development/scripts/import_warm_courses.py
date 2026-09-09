"""Import exposed expansion examples and verify the reconstructed parent season."""
import csv
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    source = REPO / "experiments/v6/sep07_compositions_v0/runs/expansion_portfolio_sep08_001/next_compile"
    output = EXP / "data/warm_courses_v1"
    output.mkdir(exist_ok=False)
    names = ["sheep8_d15_3s", "wheat12_d15_3s", "cow8_d15_3s", "strawberry8_d12_3s",
             "melon12_d12_3s", "carrot12_d15_3s", "tomato12_d15_3s", "wheat12_d15_early_inputs_3s",
             "cow8_d15_final_pruned_30s"]
    copied = []

    def copy(path, target):
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        copied.append({"source": str(path.relative_to(REPO)), "path": str(target.relative_to(EXP)),
                       "sha256": hashlib.sha256(target.read_bytes()).hexdigest()})

    rows, courses = [], []
    for name in names:
        root = source / name
        status = json.loads((root / "STATUS.json").read_text())
        courses.append({"name": name, "status": status})
        for file in ["STATUS.json", "MATCHED_RESULT.json", "compile.csv"]:
            if (root / file).is_file():
                copy(root / file, output / name / file)
        for day in sorted((root / "days").iterdir(), key=lambda p: int(p.name)):
            for path in day.iterdir():
                if path.name in ["problem.json", "actions.txt"] or path.name.startswith(("problem_h", "orders_h", "executable_h")):
                    copy(path, output / name / "days" / day.name / path.name)
            problem_path = output / name / "days" / day.name / "problem.json"
            if not problem_path.is_file():
                continue
            p = json.loads(problem_path.read_text()); hours = 23 if day.name == "29" else 24
            rows.append({"source": name + "/" + day.name, "course": name, "day": int(day.name), "active_hours": hours,
                "source_family": "exposed_expansion_world", "sources": [name + "/days/" + day.name],
                "problem": str(problem_path.relative_to(EXP)), "source_sha256": hashlib.sha256(problem_path.read_bytes()).hexdigest(),
                "physical_key": physical_key(p, hours), "observed_workers": p["worker_count"],
                "tasks": sum(len(w["actions"]) for w in p["tile_work"]), "witnesses": []})
    for name in ["sheep8_d15", "wheat12_d15", "cow8_d15", "strawberry8_d12", "melon12_d12", "carrot12_d15", "tomato12_d15"]:
        copy(source / (name + ".txt"), output / "specs" / (name + ".txt"))
    for name in ["WHEAT_COURSE_RESULT.json", "COW_COURSE_RESULT.json", "COURSE_RESULTS.json", "EXACT_PROFILES_V2.json"]:
        copy(source / name, output / "evidence" / name)
    parent = next(r for r in json.loads((output / "evidence/EXACT_PROFILES_V2.json").read_text()) if r["case"] == "parent")
    current = json.loads((EXP / "runs/warm_source_v1/REPORT.json").read_text())
    assert [current["source_action_hash"], current["rival_action_hash"]] == parent["action_hashes"]
    assert current["source_cash"] == parent["cash"] and current["rival_cash"] == parent["rival_cash"]
    days = list(csv.DictReader((EXP / "runs/warm_source_v1/DAYS.csv").open()))
    assert len(days) == 30 and all(all(row[k] == "1" for k in ["strict", "requirements", "invariants"]) and row["errors"] == "0" for row in days)
    assert sum(int(row["hire_cost"]) for row in days) == parent["profile"]["hire_cost"]
    (output / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    report = {"utc": datetime.now(timezone.utc).isoformat(), "scope": "Previously exposed motivating expansion examples; no generalization claim.",
              "parent_parity": current, "parent_hire_cost": parent["profile"]["hire_cost"], "courses": courses, "files": copied}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"copied_files": len(copied), "day_contracts": len(rows), "parent_parity": True, "courses": courses}, indent=2))


if __name__ == "__main__":
    main()
