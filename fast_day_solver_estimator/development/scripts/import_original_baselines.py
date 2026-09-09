"""Pin original estimator evidence and a self-contained executable baseline."""
import csv
import hashlib
import json
import shutil
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
SOURCE = EXP.parent / "sep07_compositions_v0"


def main():
    output = EXP / "baselines/original"
    output.mkdir(parents=True, exist_ok=False)
    evidence = EXP / "evidence/original_pipeline"
    evidence.mkdir(exist_ok=False)
    manifest = {r["path"]: r for r in map(json.loads, (SOURCE / "final/ARTIFACT_INVENTORY.jsonl").read_text().splitlines())}
    paths = {f"include/{name}.hpp": output / f"{name}.hpp" for name in ["estimate", "biology", "composition", "economics"]}
    prefix = "runs/submission_losses_sep08_001/valuation/"
    paths.update({prefix + "source/general_model.hpp": evidence / "general_model.hpp.txt",
                  prefix + "source/check_general.cpp": evidence / "check_general.cpp.txt",
                  prefix + "GENERAL_ESTIMATES.csv": evidence / "GENERAL_ESTIMATES.csv"})
    copied = []
    for source, destination in paths.items():
        data = (SOURCE / source).read_bytes()
        sha = hashlib.sha256(data).hexdigest()
        assert sha == manifest[source]["sha256"]
        shutil.copyfile(SOURCE / source, destination)
        copied.append({"source_provenance": str(SOURCE.relative_to(EXP.parents[2]) / source),
                       "path": str(destination.relative_to(EXP)), "sha256": sha})
    rows = list(csv.DictReader((evidence / "GENERAL_ESTIMATES.csv").open()))
    by_key = {(r["episode"], r["seat"], r["candidate"], r["count"], r["land_cost"], r["operation_cost"]): r for r in rows}
    compared, maximum_error = 0, 0.0
    for key, row in by_key.items():
        if row["operation_cost"] == "0":
            continue
        base = by_key[(*key[:-1], "0")]
        expected = int(row["operations"]) * float(row["operation_cost"])
        # Published text rounds financial outputs, so retain the rounding error.
        actual = float(base["own_gain"]) - float(row["own_gain"])
        maximum_error = max(maximum_error, abs(expected - actual)); compared += 1
    result = {"files": copied, "original_flat_coefficients": [0, 10, 25], "flat_comparisons": compared,
              "largest_difference_from_published_rounded_gain": maximum_error,
              "primary_historical_example": "252 added wheat operations *10 =2520; retain coefficient25 as a published sensitivity baseline.",
              "comparison_scope": "General model's exact additive labor term is coefficient * added operations. Earlier estimate_plan is a separate geometry/work baseline. Do not conflate either with the full economic forecast or recorded support."}
    (evidence / "MANIFEST.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "files"}))


if __name__ == "__main__":
    main()
