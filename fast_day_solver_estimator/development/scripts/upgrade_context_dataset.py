"""Attach explicit input-derived hire menus to existing ordinary-day evidence."""
import argparse
import csv
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

import numpy as np

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("datasets", nargs="+", type=Path)
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    merged, provenance, lines = {}, {}, []
    base_names = None
    for source in args.datasets:
        data = json.loads(source.read_text())
        assert not data.get("pending_cases"), "only completed evidence"
        assert "reference" in data, "pass original reference datasets, not an opaque merge"
        assert all(r.get("active_hours", 24) == 24 for r in data["rows"])
        if base_names is None:
            base_names = data["feature_names"][:233]
        assert data["feature_names"][:233] == base_names
        manifest = EXP / "runs" / data["reference"] / "CASES.json"
        cases = {r["id"]: r for r in json.loads(manifest.read_text())}
        provenance[str(source)] = digest(source); provenance[str(manifest)] = digest(manifest)
        for row in data["rows"]:
            case = cases[row["id"]]
            assert case.get("active_hours", 24) == 24 and case["queries"]
            problem_path = EXP / case["queries"][0]["path"]
            p = json.loads(problem_path.read_text())
            key = physical_key(p, 24)
            assert key == row["physical_key"]
            occupied = {(e["hour"], e["order_index"]) for e in p["buy_schedule"] if e["op"] != "hire"}
            menu = [(h, slot) for h in range(23) for slot in range(10) if (h, slot) not in occupied][:39]
            assert len(menu) == 39
            # Validate the actual recorded query calendars, never a source crew.
            for query in case["queries"]:
                qp = json.loads((EXP / query["path"]).read_text())
                slots = [(e["hour"], e["order_index"]) for e in qp["buy_schedule"] if e["op"] == "hire"]
                assert sorted(slots) == menu[:query["workers"] - 1]
                assert qp["worker_count"] == query["workers"]
            identity = {"obligation_key": key, "active_hours": 24, "hire_slots": menu}
            contract = hashlib.sha256(json.dumps(identity, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
            if contract not in merged:
                merged[contract] = {"id": contract[:20], "contract_key": contract, **identity,
                    "families": [], "source_ids": [], "source_datasets": [], "reference_workers": None,
                    "query_batches": [], "query_outcomes": [], "base_features": row["features"][:233],
                    "profile": "earliest", "parent_panel": "ordinary_development"}
                fields = " ".join(f"{h} {slot}" for h, slot in menu)
                lines.append(f"{contract[:20]} {problem_path} 24 39 {fields}\n")
            target = merged[contract]
            assert np.array_equal(np.float32(target["base_features"]), np.float32(row["features"][:233]))
            target["families"] = sorted(set(target["families"] + row["families"]))
            target["source_ids"].append(row["id"]); target["source_datasets"].append(str(source))
            upper = [n for n in [target["reference_workers"], row["reference_workers"]] if n is not None]
            target["reference_workers"] = min(upper) if upper else None
            batch = str(source)
            target["query_batches"].append({"source": batch, "short_sweep_workers": row["short_sweep_workers"],
                "queries": len(row["query_outcomes"]), "reference": data["reference"]})
            for query in row["query_outcomes"]:
                assert query["budget_seconds"] == 3 and query["status"] in ["FEASIBLE", "UNKNOWN"]
                target["query_outcomes"].append({**query, "batch": batch,
                    "sample_key": hashlib.sha256((batch + ":" + query["id"]).encode()).hexdigest()})
    feature_manifest = output / "FEATURES.txt"
    feature_manifest.write_text("".join(lines))
    command = ["conda", "run", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
               str(EXP / "build/planning_tool"), str(feature_manifest), str(output / "FEATURES.csv")]
    subprocess.run(command, cwd=EXP, check=True)
    features = list(csv.DictReader((output / "FEATURES.csv").open()))
    names = [k for k in features[0] if k not in ["id", "extraction_us"]]
    by_id = {r["id"]: r for r in features}
    assert names[:233] == base_names and len(by_id) == len(merged)
    for row in merged.values():
        f = by_id[row["id"]]
        assert np.array_equal(np.float32(row.pop("base_features")), np.float32([f[n] for n in base_names]))
        row["features"] = [float(f[n]) for n in names]
        row["lower_bound"] = int(float(f["planning_lower_bound"]))
        for key in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"]:
            row[key] = float(f[key])
        if row["reference_workers"] is not None:
            assert row["lower_bound"] <= row["reference_workers"] and not any(row[k] for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
    result = {"utc": datetime.now(timezone.utc).isoformat(), "schema": "explicit_calendar_training_v2",
        "scope": "Exposed development evidence with explicit input-derived earliest hire menus. No new solver calls. Repeated logged calls remain separate observations, grouped by full contract and family.",
        "target": "Reference workers are the smallest verified upper bound. Query labels describe individual three-second calls; repeated attempts are never collapsed into one success label.",
        "feature_names": names, "rows": list(merged.values()), "pending_cases": [],
        "input_sha256": provenance | {str(Path(__file__)): digest(Path(__file__)), str(EXP / "build/planning_tool"): digest(EXP / "build/planning_tool")}}
    (output / "DATASET.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"contracts": len(merged), "queries": sum(len(r["query_outcomes"]) for r in merged.values()),
                      "verified_upper": sum(r["reference_workers"] is not None for r in merged.values())}))


if __name__ == "__main__":
    main()
