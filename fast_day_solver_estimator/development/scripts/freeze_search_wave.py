"""Pin second-wave models, policy and protocol before opening holdout_a."""
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
output = EXP / "snapshots/search_wave_v2"
assert not list((EXP / "data/replay_intake").glob("holdout_a_*")), "holdout_a is already open"
assert not list((EXP / "data/replay_intake").glob("holdout_b_*")), "holdout_b is already open"
for corpus in ["fresh", "synthetic"]:
    report = json.loads((EXP / f"models/search_candidate_dev_v2/{corpus}_PARITY.json").read_text())
    assert all(r["max_absolute"] < 1e-7 for r in report["parity"].values())
output.mkdir(parents=True, exist_ok=False)
files = {}


def copy(source, target):
    source, target = EXP / source, output / target
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    if source.stat().st_mode & 0o111:
        target.chmod(0o755)
    files[str(target.relative_to(output))] = {"source": str(source.relative_to(EXP)),
        "sha256": hashlib.sha256(target.read_bytes()).hexdigest(), "bytes": target.stat().st_size}


for name in ["models.joblib", "models.py", "baseline_study.py", "DATASET.json", "RESULTS.json"]:
    copy("runs/cost_model_study_v3/" + name, "cost_study/" + name)
for name in ["models.joblib", "query_models.py", "query_study.py", "screening.py", "RESULTS.json"]:
    copy("runs/query_layout_study_v2/" + name, "query_study/" + name)
for name in ["features.hpp", "bounds.hpp", "supply_bounds.hpp"]:
    copy("include/" + name, "cpp/" + name)
copy("models/search_candidate_dev_v2/search_model.hpp", "cpp/search_model.hpp")
copy("source/predict_search.cpp", "cpp/predict_search.cpp")
copy("build/predict_search", "cpp/predict_search")
for name in ["fresh_PARITY.json", "synthetic_PARITY.json", "MANIFEST.json"]:
    copy("models/search_candidate_dev_v2/" + name, "checks/" + name)
for name in ["query_budget_study.py", "screening.py", "baseline_study.py", "prepare_reference.py", "make_layout_pools.py", "make_work_pools.py", "import_problems.py", "intake_replays.py", "export_replay_traces.py", "extract_replay_days.py", "index_fresh_days.py"]:
    copy("scripts/" + name, "policy/" + name)
copy("docs/unseen_wave_v2.md", "PROTOCOL.md")
copy("data/replay_intake/FAMILY_SPLITS.json", "FAMILY_SPLITS.json")
for name in ["estimate.hpp", "biology.hpp", "composition.hpp", "economics.hpp"]:
    copy("baselines/original/" + name, "original/" + name)
copy("evidence/original_pipeline/MANIFEST.json", "original/PROVENANCE.json")
copy("source/original_baseline.cpp", "original/original_baseline.cpp")
copy("build/original_baseline", "original/original_baseline")
copy("models/reference_ordinary_v0/reference", "reference/reference")
copy("models/reference_ordinary_v0/source.cpp", "reference/source.cpp")
assert hashlib.sha256((EXP / "build/reference").read_bytes()).hexdigest() == files["reference/reference"]["sha256"]
manifest = {"utc": datetime.now(timezone.utc).isoformat(), "status": "frozen before holdout_a download; not accepted",
    "primary": "cost_direct_extra/around with analytical and dominated-cost pruning",
    "optional_hard_case_policy": "boost/hybrid_nominal; first observed compiler failure triggers the query predictor",
    "secondary": ["boost/hybrid_cpu", "logistic/hybrid_cpu", "boost/query_nominal", "cost_direct_extra/sequential"],
    "baselines": ["original_flat_10/sequential", "original_geometry/sequential", "original_geometry/around"],
    "reference_query_range": "Task-only physical capacity lower bound through 40, independent of source workforce",
    "reference_budget_seconds": 3, "query_workers": 1, "offline_processes": 8, "active_hours": 24,
    "holdout_b": "unopened; reserved", "files": files}
(output / "FREEZE.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(json.dumps({k: v for k, v in manifest.items() if k != "files"}, indent=2))
