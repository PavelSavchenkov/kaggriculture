"""Pin third-wave predictors and selection before reserved replay intake."""
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    assert not list((EXP / "data/replay_intake").glob("holdout_b_*")), "reserved data already open"
    checks = ["runs/planning_cpp_balanced_check_v3/CHECK.json", "runs/context_cpp_check_v2/CHECK.json"]
    for name in checks:
        report = json.loads((EXP / name).read_text())
        for source, expected in report["input_sha256"].items():
            path = Path(source) if Path(source).is_absolute() else EXP / source
            assert sha(path) == expected, (source, "changed since parity check")
    output = EXP / "snapshots/planning_wave_v3"
    output.mkdir(parents=True, exist_ok=False)
    files = {}

    def copy(source, target=None):
        source = Path(source)
        target = Path(target) if target else source
        dest = output / target
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(EXP / source, dest)
        files[str(target)] = {"source": str(source), "sha256": sha(dest), "bytes": dest.stat().st_size}

    for folder, suffix in [("scripts", ".py"), ("include", ".hpp"), ("source", ".cpp")]:
        for path in sorted((EXP / folder).glob("*" + suffix)):
            copy(path.relative_to(EXP))
    for path in sorted((EXP / "baselines").rglob("*")):
        if path.is_file() and path.suffix in [".hpp", ".cpp", ".json", ".md"]:
            copy(path.relative_to(EXP))
    for source, target in [("cost_model_study_v3", "cost_study"), ("query_layout_study_v2", "query_study"),
                           ("context_calendar_augmented_v2", "context_study")]:
        for path in sorted((EXP / "runs" / source).iterdir()):
            if path.is_file() and path.name != "PREDICTIONS.jsonl":
                copy(path.relative_to(EXP), Path(target) / path.name)
    for source in ["models/search_candidate_dev_v2/search_model.hpp", "models/context_candidate_dev_v2/context_model.hpp",
                   "CMakeLists.txt", "docs/contract.md", "docs/unseen_wave_v3.md", "data/replay_intake/FAMILY_SPLITS.json", *checks,
                   "runs/warm_guided_benchmark_v1/REPORT.json", "runs/holdout_a_gate_v2/GATE.json",
                   "runs/holdout_a_refinement_cpp_v3/REPORT.json"]:
        copy(source)
    for name in ["predict_search", "predict_context", "predict_planning", "original_baseline", "reference",
                 "reference_terminal_deadlines", "extract_replay", "extract_terminal", "generate_expansions",
                 "replay_search_cpp", "replay_refinement_cpp", "warm_compile_guided", "labor_tool"]:
        copy("build/" + name)
    indices = sorted((EXP / "data").rglob("index.jsonl")) + [EXP / "runs/inventory_v0/problems.jsonl"]
    exposed, contexts, provenance = set(), set(), {}
    for index in indices:
        assert index.is_file(), index
        for row in map(json.loads, index.read_text().splitlines()):
            if "physical_key" in row: exposed.add(row["physical_key"])
            if "obligation_key" in row: exposed.add(row["obligation_key"])
            if "contract_key" in row: contexts.add(row["contract_key"])
        provenance[str(index.relative_to(EXP))] = sha(index)
    (output / "EXPOSED_KEYS.json").write_text(json.dumps({"physical_keys": sorted(exposed), "contract_keys": sorted(contexts),
        "input_sha256": provenance}, indent=2) + "\n")
    manifest = {"utc": datetime.now(timezone.utc).isoformat(), "status": "frozen before holdout_b download; not accepted",
        "protocol": "docs/unseen_wave_v3.md", "families": "holdout_b", "episodes_per_family": 4,
        "primary_ordinary_cost": "cost_direct_extra", "calendar_cost": "selected-context success curve mean workforce",
        "risk_thresholds": {"peak_success": 0.5, "point_success": 0.2},
        "ordinary_ranking": "frozen direct cost with nearby workforce order",
        "warm_integration": "not accepted; development quality gates fail",
        "reference_budget_seconds": 3, "query_workers": 1, "maximum_reference_processes": 8,
        "preexisting_physical_keys": len(exposed), "preexisting_calendar_contracts": len(contexts),
        "exposed_inventory_sha256": sha(output / "EXPOSED_KEYS.json"), "files": files}
    (output / "FREEZE.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({k: v for k, v in manifest.items() if k != "files"}, indent=2))


if __name__ == "__main__":
    main()
