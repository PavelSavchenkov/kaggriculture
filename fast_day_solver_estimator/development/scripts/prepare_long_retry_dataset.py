"""Audit conditional thirty-second retry evidence and prior observable context."""
import hashlib
import json
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path

import numpy as np

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]
DATASETS = ["runs/expansion_dataset_guided_v3/DATASET.json", "runs/expansion_owned_dataset_guided_v4/DATASET.json"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    output = EXP / "runs/long_retry_dataset_v1"; output.mkdir(exist_ok=False)
    context_path = EXP / "runs/ordinary_context_development_v2/DATASET.json"
    contexts = json.loads(context_path.read_text()); assert not contexts["pending_cases"]
    by_physical = {r["obligation_key"]: r for r in contexts["rows"]}
    assert len(by_physical) == len(contexts["rows"])
    forecast_path = EXP / "runs/context_ordinary_oof_v3/PREDICTIONS.jsonl"
    oof = defaultdict(dict)
    for row in map(json.loads, forecast_path.read_text().splitlines()):
        assert row["method"] == "calendar/boost"
        key = row["obligation_key"], row["workers"]
        family = row["family"]
        if family in oof[key]: assert abs(oof[key][family] - row["predicted_success"]) < 1e-12
        oof[key][family] = row["predicted_success"]
    observations, round_summaries, provenance = [], [], {str(context_path.relative_to(EXP)): digest(context_path), str(forecast_path.relative_to(EXP)): digest(forecast_path)}
    for relative in DATASETS:
        source = EXP / relative; data = json.loads(source.read_text()); assert not data["pending_cases"]
        provenance[relative] = digest(source)
        rows = {r["id"]: r for r in data["rows"]}
        base_manifest = EXP / "runs" / data["reference"] / "CASES.json"
        base_cases = {r["id"]: r for r in json.loads(base_manifest.read_text())}
        provenance[str(base_manifest.relative_to(EXP))] = digest(base_manifest)
        base_status_path = base_manifest.parent / "initial_3s/STATUS.json"
        base_status = json.loads(base_status_path.read_text())
        assert base_status["status"] == "completed" and base_status["active_hours"] == 24
        provenance[str(base_status_path.relative_to(EXP))] = digest(base_status_path)
        raw_long = {(r["id"], q["workers"]): q for r in rows.values() for q in r.get("adjudication_outcomes", [])}
        assert len(raw_long) == sum(len(r.get("adjudication_outcomes", [])) for r in rows.values())
        seen = set()
        for adjudication in data["adjudication_rounds"]:
            root = EXP / "runs" / adjudication["run"]
            status_path = root / adjudication["round"] / "STATUS.json"
            status = json.loads(status_path.read_text())
            assert status["status"] == "completed" and status["query_seconds"] == 30
            assert status["active_hours"] == 24 and not status["terminal_work_deadlines"]
            assert status["binary_sha256"] == base_status["binary_sha256"]
            manifest = root / "CASES.json"
            assert digest(manifest) == status["cases_sha256"] == adjudication["manifest_sha256"]
            provenance[str(manifest.relative_to(EXP))] = digest(manifest)
            provenance[str(status_path.relative_to(EXP))] = digest(status_path)
            round_rows = []
            for case in json.loads(manifest.read_text()):
                row = rows[case["id"]]; base = base_cases[case["id"]]
                context = by_physical[row["physical_key"]]
                assert case["active_hours"] == base["active_hours"] == row["active_hours"] == 24
                assert case["hiring_rule"] == base["hiring_rule"] == "earliest_prefix_39_nonpurchase_slots_v0"
                assert case["physical_key"] == base["physical_key"] == row["physical_key"]
                assert np.array_equal(np.float32(row["features"][:233]), np.float32(context["features"][:233]))
                short_calls = {q["workers"]: q for q in row["query_outcomes"]}
                base_queries = {q["workers"]: q for q in base["queries"]}
                prior_upper = case["incumbent_workers"]
                # This comes from the before-retry manifest, never the dataset's
                # current reference_workers after the long calls were observed.
                if prior_upper is not None: assert any(q["status"] == "FEASIBLE" and q["workers"] <= prior_upper for q in short_calls.values())
                for query in case["queries"]:
                    workers = query["workers"]; key = row["id"], workers
                    assert key not in seen; seen.add(key)
                    assert query == base_queries[workers]
                    path = EXP / query["path"]; assert digest(path) == query["sha256"]
                    problem = json.loads(path.read_text())
                    assert physical_key(problem, 24) == row["physical_key"] and problem["worker_count"] == workers
                    hires = sorted((e["hour"], e["order_index"]) for e in problem["buy_schedule"] if e["op"] == "hire")
                    assert hires == [tuple(v) for v in context["hire_slots"][:workers - 1]]
                    short, long = short_calls[workers], raw_long[key]
                    assert short["status"] == "UNKNOWN" and short["budget_seconds"] == 3 and long["budget_seconds"] == 30
                    assert long["status"] in ["FEASIBLE", "UNKNOWN"] and short["id"] == long["id"]
                    if long["status"] == "FEASIBLE":
                        certificate = EXP / long["certificate"]
                        assert digest(certificate) == long["certificate_sha256"]
                    family = context["families"]
                    probabilities = oof.get((row["physical_key"], workers), {})
                    missing = any(context[k] > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
                    screen = missing or workers < context["lower_bound"]
                    if screen: assert long["status"] == "UNKNOWN"
                    if len(family) == 1 and not screen: assert family[0] in probabilities
                    item = {"sample_id": hashlib.sha256(f"{adjudication['run']}:{adjudication['round']}:{long['id']}".encode()).hexdigest(),
                            "query_id": long["id"], "workers": workers, "physical_key": row["physical_key"], "contract_key": context["contract_key"],
                            "families": family, "source_families": row["families"], "panel": "owned" if "owned" in relative else "larger",
                            "round": adjudication["run"], "phase": "guided_unknown" if "guided" in adjudication["run"] else "near_upper",
                            "prior_upper_workers": prior_upper, "prior_short_success_counts": sorted(q["workers"] for q in short_calls.values() if q["status"] == "FEASIBLE"),
                            "prior_short_cpu_seconds": short["cpu_seconds"], "prior_short_wall_seconds": short["wall_seconds"],
                            "necessary_screen": screen, "lower_bound": context["lower_bound"],
                            "old_three_second_oof_probability": probabilities.get(family[0]) if len(family) == 1 else None,
                            "success": long["status"] == "FEASIBLE", "long_cpu_seconds": long["cpu_seconds"], "long_wall_seconds": long["wall_seconds"],
                            "problem": query["path"], "query_input_sha256": query["sha256"], "reference_binary_sha256": status["binary_sha256"],
                            "features": context["features"], "feature_names_sha256": hashlib.sha256(json.dumps(contexts["feature_names"]).encode()).hexdigest()}
                    observations.append(item); round_rows.append(item)
            round_summaries.append({"round": adjudication["run"], "calls": len(round_rows), "recovered": sum(r["success"] for r in round_rows),
                                    "cpu_seconds": sum(r["long_cpu_seconds"] for r in round_rows)})
        assert seen == raw_long.keys()
    assert len({r["sample_id"] for r in observations}) == len(observations) == 1333
    logical = defaultdict(list)
    for row in observations: logical[row["query_input_sha256"]].append(row)
    duplicate_outcomes = sum(len({r["success"] for r in rows}) > 1 for rows in logical.values())
    report = {"utc": datetime.now(timezone.utc).isoformat(), "schema": "conditional_long_retry_v1",
              "scope": "Exposed ordinary-day earliest-menu calls. All thirty-second outcomes follow an observed failed three-second query at the exact same input and backend binary. No warm or calendar-transfer claim.",
              "target": "Probability that a thirty-second retry certifies this failed query; UNKNOWN remains a bounded failure, not physical infeasibility.",
              "selection": "Near-upper queries and later probability-ranked unknown-case queries were selected by earlier reference policies. Retain phase and sampling counts; this is not a random population of queries.",
              "boundary": "Features are physical work, input-derived calendar, proposed workforce and explicitly prior observations. No source workforce, future upper or future schedule is an input. Phase/family/panel are audit fields, not predictors.",
              "calls": len(observations), "unique_physical_contracts": len({r["physical_key"] for r in observations}),
              "unique_exact_query_inputs": len(logical), "repeated_exact_query_inputs": sum(len(v) > 1 for v in logical.values()),
              "repeated_inputs_with_different_long_outcomes": duplicate_outcomes,
              "shared_family_calls": sum(len(r["families"]) != 1 for r in observations), "screened_calls": sum(r["necessary_screen"] for r in observations),
              "recoveries": sum(r["success"] for r in observations), "rounds": round_summaries,
              "feature_names": contexts["feature_names"], "rows": observations,
              "input_sha256": provenance | {str(Path(__file__).relative_to(EXP)): digest(Path(__file__))}}
    (output / "DATASET.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ["rows", "feature_names", "input_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
