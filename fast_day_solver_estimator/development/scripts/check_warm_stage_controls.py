"""Check complete stage-policy courses, cold retries and exported-model parity."""
import csv
import hashlib
import json
import subprocess
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def run(*command, log=None):
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", *map(str, command)],
                   cwd=EXP, stdout=log, stderr=subprocess.STDOUT if log else None, check=True)


def main():
    output = EXP / "runs/warm_stage_controls_v1"; output.mkdir(exist_ok=False)
    source = EXP / "source/warm_compile_stage.cpp"
    files = [source, EXP / "build/warm_compile_stage", EXP / "build/predict_context", EXP / "models/context_candidate_dev_v2/context_model.hpp", Path(__file__)]
    protocol = {"scope": "Two exposed development correctness controls, not a new-seed performance benchmark.",
                "cases": ["wheat12_d15", "carrot12_d15"], "seed": 1201301738, "status": "running",
                "sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    cases, expected, manifest = [], {}, []
    for spec in protocol["cases"]:
        name = f"warm_stage_controls_v1/{spec}"
        with (output / f"{spec}.log").open("w") as log:
            run("python", EXP / "scripts/run_warm_stage.py", name, EXP / f"data/warm_courses_v1/specs/{spec}.txt", log=log)
        root = output / spec
        result = json.loads((root / "RUN.json").read_text())
        if result["returncode"] == 0:
            independent = root / "INDEPENDENT.json"
            run(EXP.parents[2] / "day_solver/with_runtime.sh", EXP / "build/verify_warm_course", root / "course", protocol["seed"], independent)
            verified = json.loads(independent.read_text()); reported = json.loads((root / "course/MATCHED_RESULT.json").read_text())
            assert all(verified[k] == reported[k] for k in ["cash", "rival_cash", "produced0", "produced1"])
            result["independent"] = verified
        guidance = list(csv.DictReader((root / "course/guidance.csv").open()))
        deferred = {}
        for row in guidance:
            day, extra, retry = [int(row[k]) for k in ["day", "extra_hires", "retry"]]
            probability = float(row["predicted_success"])
            if retry:
                assert (day, extra) in deferred and probability == deferred[day, extra]
                assert row["deferred"] == "0" and float(row["prediction_wall_seconds"]) == 0
            else:
                assert bool(int(row["deferred"])) == (probability < .10)
                if row["deferred"] == "1": deferred[day, extra] = probability
            identifier = hashlib.sha256(f"{spec}:{day}:{extra}".encode()).hexdigest()[:20]
            if identifier in expected:
                assert expected[identifier] == probability
                continue
            expected[identifier] = probability
            problem = root / "course/days" / str(day) / f"problem_h{extra}.json"
            physical = json.loads(problem.read_text())
            hires = sorted((r["hour"], r["order_index"]) for r in physical["buy_schedule"] if r["op"] == "hire")
            assert len(hires) + 1 == physical["worker_count"]
            fields = " ".join(f"{h} {slot}" for h, slot in hires)
            manifest.append(f"{identifier} {problem} {23 if day == 29 else 24} {len(hires)} 0 {fields}\n")
        cases.append({"spec": spec, "returncode": result["returncode"], "independent_success": "independent" in result,
                      "predictions": len(guidance), "deferred": len(deferred), "cold_retries": sum(int(r["retry"]) for r in guidance)})
        print(cases[-1], flush=True)
    assert any(r["independent_success"] for r in cases)
    path = output / "INPUT.txt"; path.write_text("".join(manifest))
    forecasts = output / "CPP.csv"
    run(EXP.parents[2] / "day_solver/with_runtime.sh", EXP / "build/predict_context", path, forecasts, "1")
    rows = list(csv.DictReader(forecasts.open())); assert {r["id"] for r in rows} == expected.keys()
    discrepancy = max(abs(float(r["boost"]) - expected[r["id"]]) for r in rows)
    assert discrepancy < 1e-12
    protocol.update(status="completed", results=cases, unique_prediction_checks=len(expected), max_probability_discrepancy=discrepancy)
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps(protocol, indent=2))


if __name__ == "__main__":
    main()
