"""Run the predeclared second-wave reports after the complete reference sweep."""
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def run(script, *arguments):
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / script),
                    *map(str, arguments)], cwd=EXP, check=True)


def panel(kind):
    folder = {"source": "holdout_a_round1_days", "layout": "holdout_a_layout_v2", "work": "holdout_a_work_v2"}[kind]
    index = "data/" + folder + "/index.jsonl"
    dataset = "runs/holdout_a_" + kind + "_dataset_v2/DATASET.json"
    novel = "holdout_a_" + kind + "_novel_v2"
    run("project_dataset.py", "runs/holdout_a_joint_dataset_v2/DATASET.json", index, dataset)
    run("filter_novel.py", dataset, "runs/holdout_a_overlap_v2/RECORDS.jsonl", novel, *(["--pools"] if kind != "source" else []))
    for scope, path in [("full", dataset), ("novel", "runs/" + novel + "/DATASET.json")]:
        name = "holdout_a_" + kind + "_" + scope + "_evaluation_v2"
        description = "Second prospective wave; parameters and predictions frozen before new references; " + scope + " " + kind + " panel."
        if kind == "source":
            run("evaluate_frozen.py", path, "runs/holdout_a_predictions_v2/PREDICTIONS.jsonl", name, "--scope", description)
            run("error_analysis.py", "runs/" + name)
        elif kind == "layout":
            run("replay_search_cpp.py", path, index, "runs/holdout_a_predictions_v2", "runs/" + name)
            run("analyze_cpp_replay.py", "runs/" + name, path, "--scope", description)
        else:
            run("frozen_pair_analysis.py", path, index, "runs/holdout_a_predictions_v2/PREDICTIONS.jsonl", name, "--scope", description)


def main():
    status = json.loads((EXP / "runs/holdout_a_joint_reference_v2/initial_3s/STATUS.json").read_text())
    assert status["status"] == "completed", "wait for all frozen reference calls"
    run("build_dataset.py", "holdout_a_joint_reference_v2", "runs/holdout_a_inputs_v2/index.jsonl",
        "runs/holdout_a_inputs_v2/FEATURES.csv", "runs/holdout_a_predictions_v2/BOUNDS.csv", "holdout_a_joint_dataset_v2",
        "--normalized", "holdout_a_normalized_v2", "--original", "runs/holdout_a_inputs_v2/ORIGINAL.csv")
    with ThreadPoolExecutor(max_workers=3) as pool:
        results = [pool.submit(panel, kind) for kind in ["source", "layout", "work"]]
        for future in results:
            future.result()


if __name__ == "__main__":
    main()
