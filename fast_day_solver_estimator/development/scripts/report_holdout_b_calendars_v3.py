"""Assemble both completed calendar sweeps and evaluate saved forecasts."""
import hashlib
import json
import subprocess
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def run(script, *args):
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / script),
                    *map(str, args)], cwd=EXP, check=True)


def main():
    references = ["runs/holdout_b_calendar_v3_h24", "runs/holdout_b_calendar_v3_h23"]
    for reference, round_name in zip(references, ["initial_3s", "explicit_deadlines_3s"]):
        assert json.loads((EXP / reference / round_name / "STATUS.json").read_text())["status"] == "completed"
    for manifest in ["runs/holdout_b_inputs_v3/PREDICTIONS_FROZEN.json", "runs/holdout_b_secondary_query_controls_v3/MANIFEST.json"]:
        for path, expected in json.loads((EXP / manifest).read_text())["input_sha256"].items():
            assert hashlib.sha256((EXP / path).read_bytes()).hexdigest() == expected, path
    root = EXP / "runs/holdout_b_calendar_dataset_v3"; root.mkdir(exist_ok=False)
    dataset = root / "DATASET.json"
    run("build_calendar_dataset.py", "data/holdout_b_calendar_v3", dataset, "--references", *references,
        "--terminal-round", "explicit_deadlines_3s")
    point = "runs/holdout_b_planning_predictions_v3/PLANNING_PREDICTIONS.jsonl"
    novelty = "runs/holdout_b_inputs_v3/NOVELTY.jsonl"
    run("report_planning_calendars.py", dataset, point, novelty, "runs/holdout_b_calendar_accuracy_v3",
        "--controls", "data/holdout_b_calendar_v3/BASELINE_PREDICTIONS.jsonl", "data/holdout_b_calendar_v3/ORIGINAL_PREDICTIONS.jsonl",
        "--scope", "Third prospective wave, frozen primary forecasts on new-family calendars; original controls include the same necessary floor.")
    run("report_calendar_queries.py", dataset, "runs/holdout_b_planning_predictions_v3/QUERY_PREDICTIONS.jsonl", novelty,
        "runs/holdout_b_calendar_queries_v3", "--controls", "runs/holdout_b_secondary_query_controls_v3/PREDICTIONS.jsonl",
        "--scope", "Third prospective calendar wave. Primary forecasts preceded calls; secondary control parameters were frozen but their input-only forecast export followed call launch, without consulting outcomes.")
    run("report_planning_risk.py", dataset, point, "runs/holdout_b_calendar_full_risk_v3", "--scope", "Full third-wave calendar panel; exported C++ difficulty flags.")
    novel = {r["contract_key"] for r in map(json.loads, (EXP / novelty).read_text().splitlines()) if r.get("novel") and "contract_key" in r}
    data = json.loads(dataset.read_text())
    data["rows"] = [r for r in data["rows"] if r["contract_key"] in novel]
    data["projection"] = {"source": str(dataset.relative_to(EXP)), "selection": "Novel context and unexposed parent physical contract"}
    novel_path = root / "NOVEL_DATASET.json"; novel_path.write_text(json.dumps(data, indent=2) + "\n")
    run("report_planning_risk.py", novel_path, point, "runs/holdout_b_calendar_novel_risk_v3", "--scope", "Novel third-wave calendar panel; exported C++ difficulty flags.")


if __name__ == "__main__":
    main()
