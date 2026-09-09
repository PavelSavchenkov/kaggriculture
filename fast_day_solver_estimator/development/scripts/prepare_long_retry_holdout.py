"""Assemble a frozen known-certificate retry panel after the short sweep ends."""
import hashlib
import json
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    reference = EXP / "runs/holdout_b_joint_reference_v3"
    dataset = EXP / "runs/holdout_b_joint_dataset_v3/DATASET.json"
    status = json.loads((reference / "initial_3s/STATUS.json").read_text())
    assert status["status"] == "completed" and status["active_hours"] == 24
    freeze_path = EXP / "snapshots/long_retry_v1/FREEZE.json"
    freeze = json.loads(freeze_path.read_text())
    for path, sha in freeze["input_sha256"].items():
        assert hashlib.sha256((EXP / path).read_bytes()).hexdigest() == sha, path
    data = json.loads(dataset.read_text()); assert not data["pending_cases"]
    rows = {r["physical_key"]: r for r in data["rows"]}
    cases = {r["physical_key"]: r for r in json.loads((reference / "CASES.json").read_text())}
    novelty_path = EXP / "runs/holdout_b_inputs_v3/NOVELTY.jsonl"
    novelty = {tuple(r[k] for k in ["panel", "physical_key", "pool", "variant", "family"]): r["novel"]
               for r in map(json.loads, novelty_path.read_text().splitlines())}
    memberships = defaultdict(list)
    sources = [dataset, reference / "CASES.json", novelty_path, freeze_path]
    for panel in ["owned", "expansion"]:
        folder = "holdout_b_" + panel + "_v3"
        path = EXP / "data" / folder / "index.jsonl"; sources.append(path)
        for row in map(json.loads, path.read_text().splitlines()):
            if row["variant"] == "original":
                continue
            key = folder, row["physical_key"], row["pool"], row["variant"], row["source_family"]
            if novelty[key]:
                memberships[row["physical_key"]].append({"panel": panel, "family": row["source_family"], "pool": row["pool"], "variant": row["variant"]})
    predictions_path = EXP / "runs/holdout_b_planning_predictions_v3/QUERY_PREDICTIONS.jsonl"; sources.append(predictions_path)
    predictions = {}
    for row in map(json.loads, predictions_path.read_text().splitlines()):
        if row["method"] != "boost":
            continue
        old = predictions.setdefault(row["query_id"], row)
        assert old["context_id"] == row["context_id"] and old["predicted_success"] == row["predicted_success"]
    eligible = defaultdict(list); excluded = []
    for key, aliases in sorted(memberships.items()):
        row, case = rows[key], cases[key]
        families = sorted({m["family"] for m in aliases})
        reason = None
        if len(families) != 1 or len(case["source_families"]) != 1:
            reason = "shared_family"
        outcomes = {q["workers"]: q for q in row["query_outcomes"]}
        successes = [k for k, q in outcomes.items() if q["status"] == "FEASIBLE"]
        prior = min(successes, default=None)
        if reason is None and prior is None:
            reason = "no_prior_short_certificate"
        if reason is None and (row["lower_bound"] > 40 or row["deadline_missing_quantity"] > 0 or row["input_missing_quantity"] > 0):
            reason = "necessary_screen"
        queries = [] if prior is None else [q for q in case["queries"] if max(row["lower_bound"], prior - 3) <= q["workers"] < prior]
        if reason is None and not queries:
            reason = "no_nearby_failed_query"
        if reason:
            excluded.append({"physical_key": key, "reason": reason, "memberships": aliases})
            continue
        assert case["active_hours"] == 24 and case["hiring_rule"] == "earliest_prefix_39_nonpurchase_slots_v0"
        assert all(outcomes[q["workers"]]["status"] == "UNKNOWN" and outcomes[q["workers"]]["budget_seconds"] == 3 for q in queries)
        panel = "owned" if any(m["panel"] == "owned" for m in aliases) else "expansion"
        state = {**case, "queries": queries, "incumbent_workers": prior, "verified_lower_bound": row["lower_bound"],
                 "retry_panel": panel, "retry_family": families[0], "retry_memberships": aliases}
        eligible[panel, families[0]].append(state)
    selected = []
    for (panel, family), values in sorted(eligible.items()):
        ordered = sorted(values, key=lambda r: hashlib.sha256(("long_retry_v1:" + r["physical_key"]).encode()).hexdigest())
        selected.extend(ordered[:12])
        excluded.extend({"physical_key": r["physical_key"], "reason": "fixed_stratum_cap", "memberships": r["retry_memberships"]} for r in ordered[12:])
    options = []
    for state in selected:
        for query in state["queries"]:
            path = EXP / query["path"]
            assert hashlib.sha256(path.read_bytes()).hexdigest() == query["sha256"]
            identifier = state["id"] + "_w" + f"{query['workers']:02}"
            prediction = predictions[identifier]
            options.append({"query_id": identifier, "physical_key": state["physical_key"], "workers": query["workers"],
                            "prior_upper_workers": state["incumbent_workers"], "probability": prediction["predicted_success"],
                            "context_id": prediction["context_id"], "panel": state["retry_panel"], "family": state["retry_family"],
                            "query_input_sha256": query["sha256"], "query_path": query["path"]})
    output = EXP / "runs/holdout_b_long_retry_reference_v1"; output.mkdir(exist_ok=False)
    (output / "CASES.json").write_text(json.dumps(selected, indent=2) + "\n")
    (output / "OPTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in options))
    (output / "EXCLUDED.json").write_text(json.dumps(excluded, indent=2) + "\n")
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "prepared_before_long_outcomes", "cases": len(selected),
                "queries": len(options), "strata": [{"panel": p, "family": f, "eligible": len(v), "selected": min(12, len(v))} for (p, f), v in sorted(eligible.items())],
                "exclusions": dict(Counter(r["reason"] for r in excluded)), "query_budget_seconds": 30, "threads_per_query": 1,
                "target": "Exact same-input thirty-second retries after observed three-second failures, with a known short-sweep certificate.",
                "boundary": "Short outcomes define the caller state. Weights, utility threshold and selection algorithm were frozen earlier. No long outcomes exist yet. Initially unknown cases are excluded from this optional refinement scope and counted explicitly.",
                "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
                "reference_binary_sha256": status["binary_sha256"]}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps({k: protocol[k] for k in ["cases", "queries", "strata", "exclusions"]}))


if __name__ == "__main__":
    main()
