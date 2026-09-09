"""Assemble the frozen third-wave inputs without reading any solve outcomes."""
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]
SNAPSHOT = EXP / "snapshots/planning_wave_v3"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def execute(*args):
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", *map(str, args)], cwd=EXP, check=True)


def main():
    frozen = json.loads((SNAPSHOT / "FREEZE.json").read_text())
    for target, info in frozen["files"].items():
        assert sha(SNAPSHOT / target) == info["sha256"], target
        assert sha(EXP / info["source"]) == info["sha256"], (target, "live source changed")
    exposed = json.loads((SNAPSHOT / "EXPOSED_KEYS.json").read_text())
    physical_exposed, calendar_exposed = set(exposed["physical_keys"]), set(exposed["contract_keys"])
    output = EXP / "runs/holdout_b_inputs_v3"; output.mkdir(exist_ok=False)
    ordinary_panels = ["holdout_b_round1_days", "holdout_b_layout_v3", "holdout_b_work_v3", "holdout_b_owned_v3", "holdout_b_expansion_v3"]
    ordinary, novelty, inputs, summary = [], [], {}, []
    for panel in ordinary_panels:
        path = EXP / "data" / panel / "index.jsonl"; inputs[str(path.relative_to(EXP))] = sha(path)
        rows = [json.loads(line) for line in path.read_text().splitlines()]
        parent_keys = {r["pool"]: r["physical_key"] for r in rows if r.get("variant") == "original"}
        for row in rows:
            key = row["physical_key"]
            assert physical_key(json.loads((EXP / row["problem"]).read_text())) == key
            parent = parent_keys.get(row.get("pool"), key)
            novelty.append({"panel": panel, "id": key[:20], "physical_key": key, "family": row["source_family"],
                            "pool": row.get("pool"), "variant": row.get("variant"),
                            "exposed_overlap": key in physical_exposed, "parent_exposed": parent in physical_exposed,
                            "novel": key not in physical_exposed and parent not in physical_exposed})
        ordinary.extend(rows)
        selected = [r for r in novelty if r["panel"] == panel]
        summary.append({"panel": panel, "records": len(rows), "physical": len({r["physical_key"] for r in rows}),
                        "novel_physical": len({r["physical_key"] for r in selected if r["novel"]}),
                        "pools": len(parent_keys), "novel_parents": sum(k not in physical_exposed for k in parent_keys.values())})
    joint = output / "ORDINARY_INDEX.jsonl"
    joint.write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in ordinary))
    unique = {r["source_sha256"]: r for r in ordinary}
    (output / "ORDINARY_FEATURES.txt").write_text("".join(f"{key[:20]} {EXP / row['problem']}\n" for key, row in sorted(unique.items())))
    grouped = defaultdict(list)
    for row in ordinary: grouped[row["physical_key"]].append(row)
    contexts, lines = {}, {}

    def add_context(p, path, key, hours, menu, families, profile, panel, source_id):
        identity = {"obligation_key": key, "active_hours": hours, "hire_slots": menu}
        contract = hashlib.sha256(json.dumps(identity, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        assert physical_key(p, hours) == key
        if contract not in contexts:
            contexts[contract] = {"id": contract[:20], "contract_key": contract, **identity,
                "families": [], "source_ids": [], "source_panels": [], "profile": profile, "parent_panel": panel,
                "query_outcomes": [{"id": f"{contract[:20]}_w{k:02}", "workers": k} for k in range(1, len(menu) + 2)]}
            slots = " ".join(f"{h} {s}" for h, s in menu)
            lines[contract] = f"{contract[:20]} {path} {hours} {len(menu)} {slots}\n"
        row = contexts[contract]
        row["families"] = sorted(set(row["families"]) | set(families))
        row["source_ids"] = sorted(set(row["source_ids"]) | {source_id})
        row["source_panels"] = sorted(set(row["source_panels"]) | {panel})
        return contract

    for key, rows in sorted(grouped.items()):
        path = EXP / rows[0]["problem"]; p = json.loads(path.read_text())
        occupied = {(e["hour"], e["order_index"]) for e in p["buy_schedule"] if e["op"] != "hire"}
        menu = [(h, slot) for h in range(23) for slot in range(10) if (h, slot) not in occupied][:39]
        assert len(menu) == 39
        add_context(p, path, key, 24, menu, {r["source_family"] for r in rows}, "earliest", "ordinary", key[:20])
    path = EXP / "data/holdout_b_calendar_v3/index.jsonl"; inputs[str(path.relative_to(EXP))] = sha(path)
    for row in map(json.loads, path.read_text().splitlines()):
        key, hours = row["obligation_key"], row["active_hours"]
        problem = EXP / row["problem"]
        contract = add_context(json.loads(problem.read_text()), problem, key, hours, row["hire_slots"],
                               [row["source_family"]], row["profile"], "calendar", row["id"])
        assert contract == row["contract_key"]
        novelty.append({"panel": "holdout_b_calendar_v3", "id": row["id"], "physical_key": key,
                        "contract_key": contract, "family": row["source_family"], "profile": row["profile"], "active_hours": hours,
                        "exposed_overlap": contract in calendar_exposed, "parent_exposed": key in physical_exposed,
                        "novel": contract not in calendar_exposed and key not in physical_exposed})
    (output / "FEATURES.txt").write_text("".join(lines.values()))
    execute(EXP.parents[2] / "day_solver/with_runtime.sh", EXP / "build/planning_tool", output / "FEATURES.txt", output / "FEATURES.csv")
    features = list(csv.DictReader((output / "FEATURES.csv").open()))
    names = [key for key in features[0] if key not in ["id", "extraction_us"]]
    by_id = {r["id"]: r for r in features}; assert len(by_id) == len(contexts)
    for row in contexts.values():
        feature = by_id[row["id"]]
        row["features"] = [float(feature[n]) for n in names]
        row["lower_bound"] = int(float(feature["planning_lower_bound"]))
        for name in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"]:
            row[name] = float(feature[name])
    data = {"utc": datetime.now(timezone.utc).isoformat(), "schema": "unlabeled_planning_inputs_v3",
            "scope": "Input-only forecast catalog. query_outcomes contains proposed action IDs and workforce only, with no outcome or cost label.",
            "feature_names": names, "rows": list(contexts.values()), "pending_cases": [], "input_sha256": inputs}
    (output / "INPUT_DATASET.json").write_text(json.dumps(data, indent=2) + "\n")
    (output / "NOVELTY.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in novelty))
    report = {"utc": datetime.now(timezone.utc).isoformat(), "panels": summary, "joint_physical": len(grouped),
              "joint_source_hashes": len(unique), "contexts": len(contexts),
              "calendar_novel": sum(r["novel"] for r in novelty if r["panel"] == "holdout_b_calendar_v3"),
              "boundary": "No reference files or candidate answer fields read for prediction or selection.",
              "input_sha256": inputs | {"freeze": sha(SNAPSHOT / "FREEZE.json"), "script": sha(Path(__file__)), "planning_tool": sha(EXP / "build/planning_tool")}}
    (output / "PREPARED.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
