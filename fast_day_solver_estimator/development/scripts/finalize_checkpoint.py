"""Record the stopped research checkpoint and rename the experiment once."""
import csv
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]
PACKAGE = REPO / "fast_day_solver_estimator"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    stopped = json.loads((EXP / "evidence/finalization/STOPPED.json").read_text())
    assert not stopped["active_processes_remaining"]
    references = []
    for name, round_name in [("holdout_b_joint_reference_v3", "initial_3s"), ("holdout_b_calendar_v3_h24", "initial_3s"),
                             ("holdout_b_calendar_v3_h23", "explicit_deadlines_3s")]:
        root = EXP / "runs" / name
        cases = json.loads((root / "CASES.json").read_text())
        completed = set()
        for path in (root / round_name).glob("worker*/results.jsonl"):
            for line in path.read_text().splitlines(keepends=True):
                if line.endswith("\n"):
                    record = json.loads(line); assert record["id"] not in completed; completed.add(record["id"])
        expected = {f"{case['id']}_w{query['workers']:02}" for case in cases for query in case["queries"]}
        assert completed <= expected
        references.append({"run": name, "round": round_name, "expected_calls": len(expected), "completed_calls": len(completed),
                           "status": "completed" if expected == completed else "interrupted_by_user_finalization",
                           "missing_query_ids": sorted(expected - completed)})
    benchmarks = []
    for name, expected in [("warm_seed_benchmark_v2", 112), ("warm_stage_seed_benchmark_v3", 112), ("warm_shop_benchmark_v1", 252)]:
        root = EXP / "runs" / name
        paths = sorted((root / "jobs").glob("*/*/RUN.json"))
        complete = [p for p in paths if "returncode" in json.loads(p.read_text())]
        benchmarks.append({"run": name, "expected_child_runs": expected, "completed_child_runs": len(complete),
                           "status": "completed" if len(complete) == expected else "interrupted_by_user_finalization",
                           "completed_child_sha256": {str(p.relative_to(EXP)): sha(p) for p in complete}})
    seed_rows = list(csv.DictReader((EXP / "runs/seed_suffix_controls_v1/EXACT.csv").open()))
    assert len(seed_rows) == 240 and all(r["verified_workers"] == r["seed_suffix_bound"] for r in seed_rows)
    result = {"finalized_utc": datetime.now(timezone.utc).isoformat(), "reason": stopped["reason"],
              "status": "research_finalized_at_user_request", "background_jobs_running": False,
              "accepted_scopes": ["search_v2_cost_only_h24_cold", "warm_defer_002_fixed_fixture_seed_transfer"],
              "general_estimator_accepted": False, "references": references, "benchmarks": benchmarks,
              "long_retry_holdout": "queued but not started; no prospective long-retry acceptance",
              "seed_suffix_prototype": {"exact_constructed_cases": 240, "controls": 1200, "accepted": False,
                                         "scope": "Constructed late-seed plant/water cases only; not integrated into frozen prediction."},
              "in_flight_policy": "User-interrupted invocations are not solver failures and do not supply completed timing observations. Prior native failures remain failures.",
              "path_policy": "Preserve historical bytes and hashes, including original absolute paths. Use current location or a restored workspace for new runs; never rewrite frozen evidence in place."}
    (EXP / "FINAL_CHECKPOINT.json").write_text(json.dumps(result, indent=2) + "\n")
    shutil.copyfile(EXP / "FINAL_CHECKPOINT.json", PACKAGE / "evidence/FINAL_CHECKPOINT.json")
    summaries = {"references": [{k:v for k,v in r.items() if k != "missing_query_ids"} for r in references],
                 "benchmarks": [{k:v for k,v in r.items() if k != "completed_child_sha256"} for r in benchmarks]}
    (PACKAGE / "evidence/CHECKPOINT_SUMMARY.json").write_text(json.dumps(summaries, indent=2) + "\n")
    old_status = json.loads((EXP / "STATUS.json").read_text())
    (EXP / "evidence/finalization/STATUS_BEFORE_FINALIZATION.json").write_text(json.dumps(old_status, indent=2) + "\n")
    old_status.update(status="finalized", phase="portable_package_handoff", finalized_utc=result["finalized_utc"],
                      next_review_utc=None, final_checkpoint="FINAL_CHECKPOINT.json", final_package="../../../fast_day_solver_estimator")
    (EXP / "STATUS.json").write_text(json.dumps(old_status, indent=2) + "\n")
    prior_readme = (EXP / "README.md").read_text()
    (EXP / "evidence/finalization/README_BEFORE_FINALIZATION.md").write_text(prior_readme)
    (EXP / "README.md").write_text("# Fast day-solver estimator: finalized experiment\n\n"
        "Research started September 9, 2026 at 00:56:39 UTC and ended early at the user's request to package the work and quit. All research background jobs are stopped.\n\n"
        "The complete reusable package is `../../../fast_day_solver_estimator/`. This experiment retains all raw data, verified schedules, predictions, models, attempted references, failed gates, partial tests, and the research history. Its complete checkpoint is also included in that package's research archive.\n\n"
        "Accepted scopes: ordinary H24 earliest-menu cold ordering, and .02 warm deferral on disjoint seeds with the fixed source/rival/shop fixture. There is no accepted general estimator. Complete compiler CPU improves 23.00% on 56 new-seed pairs; separate cold-search time-to-best improves 87.76% on 46 novel pools. Individual hire bills can regress; costly additions remain a weakness.\n\n"
        "Read `FINAL_CHECKPOINT.json` for completed and interrupted counts. Historical run statuses and documents are preserved as evidence of their original state; this final checkpoint is authoritative about whether jobs are still running. Do not promote partial tests.\n\n"
        "Final formulation, usage, metrics, positive/negative learnings, promotion rules, and resumption instructions are copied into `docs/final/`. The seed-suffix prototype has constructed controls only and is not integrated into the frozen estimator.\n\n"
        "No Git commands or submissions were made. The user explicitly authorized cataloging required fixture agents; the final package references their entries under `agents/external/`. Historical absolute paths retain the original experiment name for hash provenance; `RENAMED.json` records the location change.\n")
    target = EXP.parent / "fast_day_solver_estimator"
    assert target != EXP and not target.exists()
    rename = {"utc": datetime.now(timezone.utc).isoformat(), "old_path": str(EXP), "new_path": str(target),
              "scope": "Directory rename only. Historical hashed evidence is not rewritten."}
    (EXP / "RENAMED.json").write_text(json.dumps(rename, indent=2) + "\n")
    shutil.copyfile(EXP / "RENAMED.json", PACKAGE / "evidence/RENAMED.json")
    EXP.rename(target)
    print(json.dumps(summaries, indent=2))


if __name__ == "__main__":
    main()
