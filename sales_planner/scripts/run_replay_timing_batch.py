"""Batch full C++ replay validation, retaining incomplete and changed cases."""
import argparse
import hashlib
import json
import subprocess
import time
from pathlib import Path

from summarize_timing_variants import describe, utility


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    reference = protocol.get("reference", "wait_history_floor")
    assert reference in protocol["variants"]
    identity = {"binary": str(args.binary.resolve()), "sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                "protocol": protocol}
    identity_path = args.directory / "RUNNER.json"
    if identity_path.exists():
        assert json.loads(identity_path.read_text()) == identity
    else:
        identity_path.write_text(json.dumps(identity, indent=2) + "\n")
    statuses, rows, raw, originals = [], [], {}, {}
    for variant, flags in protocol["variants"].items():
        command = [str(args.binary), "--source-actions", *flags, *[str(EXP / p) for p in protocol["cases"]]]
        status_path = args.directory / (variant + ".status.json")
        output = args.directory / (variant + ".jsonl")
        if status_path.exists():
            status = json.loads(status_path.read_text())
        else:
            started = time.perf_counter()
            with output.open("w") as out, (args.directory / (variant + ".events.jsonl")).open("w") as err:
                result = subprocess.run(command, stdout=out, stderr=err)
            status = {"variant": variant, "command": command, "returncode": result.returncode,
                      "seconds": time.perf_counter() - started}
            status_path.write_text(json.dumps(status, indent=2) + "\n")
        statuses.append(status)
        values = [json.loads(line) for line in output.read_text().splitlines()]
        bases = {(r["episode"], r["seat"]): r for r in values if r["policy"] == "original"}
        for new in values:
            if new["policy"] == "original":
                continue
            assert new["policy"] == variant
            key = new["episode"], new["seat"]
            old = bases[key]
            signature = {k: v for k, v in old.items() if k not in ("seconds", "history_underestimates")}
            if key in originals:
                assert originals[key] == signature
            originals[key] = signature
            changed = [k for k in ("own_faults", "rival_faults", "own_requested", "rival_requested", "own_produced", "rival_produced",
                       "own_discarded", "rival_discarded", "own_final_stock", "rival_final_stock",
                       "own_source_contract_exceptions", "rival_source_contract_exceptions") if old[k] != new[k]]
            changed += [k for k in ("own_work_differences", "rival_work_differences", "shop_differences") if new[k]]
            seed_fields = [k for k in ("own_final_seeds", "rival_final_seeds") if k in old]
            changed += [k for k in seed_fields if old[k] != new[k]]
            row = {"episode": key[0], "seat": key[1], "variant": variant,
                   "margin_gain": new["margin"] - old["margin"], "cash_gain": new["cash"] - old["cash"],
                   "rival_cash_gain": new["rival_cash"] - old["rival_cash"],
                   "utility_gain_pp": 100 * (utility(new) - utility(old)), "changed_state": changed,
                   "history_underestimates": new["history_underestimates"], "decisions": new["delay_decisions"],
                   "delayed_units": new["delayed_units"], "prediction_error": new["cash"] - old["cash"] - new["predicted_own_gain"],
                   "ending_seed_differences": [k for k in seed_fields if old[k] != new[k]],
                   "non_seed_work_difference": bool((new["own_work_mask"] | new["rival_work_mask"]) & ~2) if "own_work_mask" in new else None,
                   "seed_units_removed": new.get("seed_units_removed"), "seed_orders_changed": new.get("seed_orders_changed")}
            rows.append(row); raw[variant, *key] = new
        print(json.dumps(status), flush=True)
    summaries, comparisons = [], []
    for variant in protocol["variants"]:
        chosen = [r for r in rows if r["variant"] == variant]
        expected = len(protocol["cases"]) * 2
        summaries.append({"variant": variant, "expected_cases": expected, "incomplete_cases": expected - len(chosen),
                          "scope": "All-case promotion requires complete execution and the declared noncash checks.", **describe(chosen)})
        if variant == reference:
            continue
        paired = []
        for row in chosen:
            key = row["episode"], row["seat"]
            if (reference, *key) not in raw:
                continue
            a, b = raw[reference, *key], raw[variant, *key]
            paired.append(row | {"margin_gain": b["margin"] - a["margin"], "cash_gain": b["cash"] - a["cash"],
                                 "utility_gain_pp": 100 * (utility(b) - utility(a))})
        comparisons.append({"variant": variant, "reference": reference, **describe(paired)})
    for name, data in [("RUN_STATUS", statuses), ("SUMMARY", summaries), ("COMPARISONS", comparisons), ("PAIRED_GAMES", rows),
                       ("NEGATIVE_OR_CHANGED", [r for r in rows if r["margin_gain"] < 0 or r["changed_state"] or r["history_underestimates"]]),
                       ("PREDICTION_ERRORS", [r for r in rows if r["prediction_error"]])]:
        (args.directory / (name + ".json")).write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps({"summaries": summaries, "comparisons": comparisons}, indent=2))


if __name__ == "__main__":
    main()
