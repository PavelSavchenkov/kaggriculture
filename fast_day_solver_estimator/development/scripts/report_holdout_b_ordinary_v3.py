"""Report the frozen third-wave ordinary panels after every query completes."""
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def run(script, *arguments):
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / script),
                    *map(str, arguments)], cwd=EXP, check=True)


def panel(kind):
    folder = {"source": "holdout_b_round1_days", "layout": "holdout_b_layout_v3", "work": "holdout_b_work_v3",
              "owned": "holdout_b_owned_v3", "expansion": "holdout_b_expansion_v3"}[kind]
    for scope in ["full", "novel"]:
        output = "runs/holdout_b_" + kind + "_" + scope + "_v3"
        run("project_planning_panel.py", "runs/holdout_b_joint_dataset_v3/DATASET.json", "data/" + folder + "/index.jsonl",
            "runs/holdout_b_inputs_v3/NOVELTY.jsonl", output, *(["--novel"] if scope == "novel" else []),
            *(["--whole-pools"] if kind == "layout" else []))
        dataset, index = output + "/DATASET.json", output + "/INDEX.jsonl"
        name = "holdout_b_" + kind + "_" + scope + "_evaluation_v3"
        description = "Third prospective wave, frozen before new-family reference outcomes; " + scope + " " + kind + " panel."
        if kind == "source":
            run("evaluate_frozen.py", dataset, "runs/holdout_b_cost_predictions_v3/PREDICTIONS.jsonl", name, "--scope", description)
            run("error_analysis.py", "runs/" + name)
        elif kind == "layout":
            run("replay_search_cpp.py", dataset, index, "runs/holdout_b_cost_predictions_v3", "runs/" + name)
            run("analyze_cpp_replay.py", "runs/" + name, dataset, "--scope", description)
            run("report_cold_gate_v3.py", "runs/" + name, "--scope", description + " Ordinary cold-search gate.")
            run("replay_refinement_cpp.py", dataset, index, "runs/holdout_b_cost_predictions_v3/PREDICTIONS.jsonl",
                "runs/holdout_b_planning_predictions_v3/QUERY_PREDICTIONS.jsonl", "runs/holdout_b_layout_" + scope + "_refinement_v3",
                "--scope", description + " Observable stopping is a secondary compute/bill tradeoff.")
        else:
            run("frozen_pair_analysis.py", dataset, index, "runs/holdout_b_cost_predictions_v3/PREDICTIONS.jsonl", name,
                "--target-field", "short_sweep_workers", "--scope", description)
            run("report_marginal_gate_v3.py", "runs/" + name, "--scope", description + " Fixed family-macro gate; removals are a separate diagnostic.")
        if kind in ["source", "owned", "expansion"]:
            run("report_planning_risk.py", dataset, "runs/holdout_b_planning_predictions_v3/PLANNING_PREDICTIONS.jsonl",
                "runs/holdout_b_" + kind + "_" + scope + "_risk_v3", "--scope", description + " Actual C++ difficulty flags and adaptive point costs.")
    if kind == "source":
        for model, predictions in [("old", "holdout_b_cost_predictions_v3"), ("context", "holdout_b_planning_predictions_v3")]:
            run("evaluate_query_forecasts.py", "runs/holdout_b_source_novel_v3/DATASET.json", "runs/" + predictions + "/QUERY_PREDICTIONS.jsonl",
                "runs/holdout_b_source_novel_query_" + model + "_v3", "--scope", "Third frozen wave, novel source calls; " + model)


def main():
    status = json.loads((EXP / "runs/holdout_b_joint_reference_v3/initial_3s/STATUS.json").read_text())
    assert status["status"] == "completed", "wait for every fixed query"
    frozen = json.loads((EXP / "runs/holdout_b_inputs_v3/PREDICTIONS_FROZEN.json").read_text())
    for path, expected in frozen["input_sha256"].items():
        assert hashlib.sha256((EXP / path).read_bytes()).hexdigest() == expected, path
    run("build_dataset.py", "holdout_b_joint_reference_v3", "runs/holdout_b_inputs_v3/ORDINARY_INDEX.jsonl",
        "runs/holdout_b_inputs_v3/ORDINARY_FEATURES.csv", "runs/holdout_b_cost_predictions_v3/BOUNDS.csv", "holdout_b_joint_dataset_v3",
        "--original", "runs/holdout_b_inputs_v3/ORDINARY_ORIGINAL.csv")
    with ThreadPoolExecutor(max_workers=3) as pool:
        tasks = [pool.submit(panel, kind) for kind in ["source", "layout", "work", "owned", "expansion"]]
        for task in tasks: task.result()


if __name__ == "__main__":
    main()
