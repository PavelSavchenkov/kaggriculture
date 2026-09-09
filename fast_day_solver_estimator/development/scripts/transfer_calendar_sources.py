"""Replay existing source certificates under each exact calendar before reuse."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("sources", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    data = json.loads(args.dataset.read_text())
    assert not data["pending_cases"]
    sources = {r["physical_key"]: r for r in json.loads(args.sources.read_text())["rows"]}
    cases = {c["id"]: c for c in json.loads((args.reference / "CASES.json").read_text())}
    candidates, hours_set = [], set()
    for row in data["rows"]:
        source = sources.get(row["obligation_key"])
        if not source or source.get("source_certificate") is None:
            continue
        k = source["source_workers"]
        query = next((q for q in cases[row["id"]]["queries"] if q["workers"] == k), None)
        if query is None:
            continue
        hours_set.add(row["active_hours"])
        candidates.append({"id": row["id"], "workers": k, "problem": query["path"], "certificate": source["source_certificate"]})
    assert len(hours_set) == 1
    hours = next(iter(hours_set))
    manifest = output / "WITNESSES.txt"
    manifest.write_text("".join(f"{r['id']} {EXP / r['problem']} {EXP / r['certificate']}\n" for r in candidates))
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/horizon_tool"), "audit", str(manifest), str(output / "AUDIT.csv"), str(hours)], check=True)
    audits = {r["id"]: r for r in csv.DictReader((output / "AUDIT.csv").open())}
    rows = {r["id"]: r for r in data["rows"]}; changed, valid_count = [], 0
    for candidate in candidates:
        check = audits[candidate["id"]]
        valid = all(check[k] == "1" for k in ["strict", "requirements", "invariants"]) and check["errors"] == "0" and (hours == 24 or check["terminal_phase_empty"] == "1")
        candidate["valid_under_this_calendar"] = valid
        if not valid: continue
        valid_count += 1; row = rows[candidate["id"]]
        assert candidate["workers"] >= row["lower_bound"]
        candidate["certificate_sha256"] = hashlib.sha256((EXP / candidate["certificate"]).read_bytes()).hexdigest()
        row.setdefault("transferred_source_certificates", []).append(candidate)
        if row["reference_workers"] is None or candidate["workers"] < row["reference_workers"]:
            changed.append({"id": row["id"], "before": row["reference_workers"], "after": candidate["workers"], "profile": row["profile"]})
            row["reference_workers"] = candidate["workers"]
    data["source_transfer"] = {"scope": "Existing source witnesses independently replayed against this exact query's hire calendar. Short-sweep workers and all query outcomes remain unchanged.",
                               "attempted": len(candidates), "valid": valid_count, "changed": changed,
                               "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.sources, args.reference / "CASES.json", Path(__file__)]}}
    (output / "DATASET.json").write_text(json.dumps(data, indent=2) + "\n")
    (output / "CERTIFICATES.json").write_text(json.dumps(candidates, indent=2) + "\n")
    print(json.dumps(data["source_transfer"], indent=2))


if __name__ == "__main__":
    main()
