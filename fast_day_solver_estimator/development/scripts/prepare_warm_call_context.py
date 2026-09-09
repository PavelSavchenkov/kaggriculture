"""Expose only proposed call inputs from a completed warm compiler catalog."""
import argparse
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("catalog", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.catalog = args.catalog.resolve(); args.output = args.output.resolve()
    protocol = json.loads((args.catalog / "PROTOCOL.json").read_text()); assert protocol["status"] == "completed"
    args.output.mkdir(exist_ok=False)
    rows, lines, groups = [], [], defaultdict(list)
    for path in sorted(args.catalog.glob("*/course/query_calls.csv")):
        for raw in csv.DictReader(path.open()):
            day, extra, workers = [int(raw[k]) for k in ["day", "extra_hires", "workers"]]
            hours = 23 if day == 29 else 24
            problem = path.parent / "days" / str(day) / f"problem_h{extra}.json"
            p = json.loads(problem.read_text()); assert p["worker_count"] == workers
            hires = sorted((e["hour"], e["order_index"]) for e in p["buy_schedule"] if e["op"] == "hire")
            assert len(hires) == workers - 1
            name = path.parent.parent.name
            identifier = hashlib.sha256(f"{name}:{day}:{extra}".encode()).hexdigest()[:20]
            key = physical_key(p, hours)
            row = {"id": identifier, "course": name, "day": day, "extra_hires": extra, "workers": workers,
                "active_hours": hours, "physical_key": key, "problem": str(problem.relative_to(EXP)),
                "hire_slots": hires, "reused": bool(int(raw["reused"])), "physical_certificate": bool(int(raw["physical_certificate"])),
                "endpoint": bool(int(raw["endpoint"])), "repair_called": bool(int(raw["repair_called"])),
                "repair_succeeded": bool(int(raw["repair_succeeded"])), "cold_called": bool(int(raw["cold_called"]))}
            for column in ["cpu_seconds", "wall_seconds", "reuse_cpu_seconds", "repair_cpu_seconds", "cold_cpu_seconds"]:
                row[column] = float(raw[column]); assert row[column] >= 0
            assert row["cpu_seconds"] + 1e-6 >= sum(row[k] for k in ["reuse_cpu_seconds", "repair_cpu_seconds", "cold_cpu_seconds"])
            rows.append(row); groups[name, day].append(row)
            fields = " ".join(f"{h} {slot}" for h, slot in hires)
            # All selected hires are explicit commitments in this single-call
            # manifest. They were proposed before its outcome, not read from a
            # candidate's successful schedule. There is no optional suffix.
            lines.append(f"{identifier} {problem.resolve()} {hours} {len(hires)} 0 {fields}\n")
    for group in groups.values():
        assert sorted(r["extra_hires"] for r in group) == list(range(7))
        assert len({r["physical_key"] for r in group}) == 1
        base = next(r for r in group if r["extra_hires"] == 0)
        fixed = set(map(tuple, base["hire_slots"]))
        for row in group:
            selected = set(map(tuple, row["hire_slots"]))
            assert fixed <= selected and len(selected - fixed) == row["extra_hires"]
            row["committed_workers"] = base["workers"]
    manifest = args.output / "INPUT.txt"; manifest.write_text("".join(lines))
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/predict_context"), str(manifest), str(args.output / "CPP_PREDICTIONS.csv"), "1"], cwd=EXP, check=True)
    forecasts = {r["id"]: r for r in csv.DictReader((args.output / "CPP_PREDICTIONS.csv").open())}
    assert set(forecasts) == {r["id"] for r in rows}
    for row in rows:
        prediction = forecasts[row["id"]]
        assert int(prediction["workers"]) == row["workers"]
        for key in ["boost", "logistic", "cpu", "feature_cpu_us", "query_cpu_us"]:
            row["predicted_" + key] = float(prediction[key])
        row["lower_bound"] = int(prediction["lower_bound"])
    report = {"scope": "Complete exposed warm-call catalog. Existing cold context models evaluated without retraining; no next-query decision has used these outcomes yet.",
        "boundary": "Physical obligations plus the explicitly proposed workforce and hire times enter prediction. Reuse/repair/cold outcomes and source route are evidence only. No candidate certificate or answer enters the model.",
        "target": "Full warm compiler success requires both a strict physical certificate and the exact full-game endpoint. Complete per-call CPU includes all its stages. This differs from the three-second cold model target.",
        "groups": len(groups), "calls": len(rows), "rows": rows,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.catalog / "PROTOCOL.json", manifest, Path(__file__),
            EXP / "build/predict_context", EXP / "models/context_candidate_dev_v2/context_model.hpp"]}}
    (args.output / "DATASET.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"groups": len(groups), "calls": len(rows), "endpoint_successes": sum(r["endpoint"] for r in rows)}))


if __name__ == "__main__":
    main()
