"""Compare two finance candidates on the same originally controlled games."""
import argparse
import json
from pathlib import Path

from summarize_course_timing import describe


def key(row):
    return tuple(row[k] for k in ("seed", "seat", "branch", "panel", "opponent"))


def utility(row):
    return float(row["margin"] > 0) + 0.5 * (row["margin"] == 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    reference = {key(r): r for r in json.loads((args.reference / "PAIRED_GAMES.json").read_text())}
    candidate = {key(r): r for r in json.loads((args.candidate / "PAIRED_GAMES.json").read_text())}
    assert reference.keys() == candidate.keys()
    paired = []
    for case in sorted(reference):
        before, after = reference[case], candidate[case]
        for field in ("cash", "rival_cash", "own_action_hash", "rival_action_hash", "produced", "stock"):
            assert before["repair"][field] == after["repair"][field], (case, field)
        a, b = before["history"], after["history"]
        # Reuse the existing statistical helper's internal reference/candidate
        # slots; exported summaries rename them below.
        paired.append({k: after[k] for k in ("seed", "seat", "branch", "panel", "opponent")} |
                      {"repair": a, "history": b, "margin_gain": b["margin"] - a["margin"],
                       "cash_gain": b["cash"] - a["cash"],
                       "utility_gain_pp": 100 * (utility(b) - utility(a)),
                       "both_match_original_physical_state": not (a["own_changed_turns"] or b["own_changed_turns"] or
                           a["rival_changed_turns"] or b["rival_changed_turns"])})

    def summarize(rows):
        result = describe(rows)
        result["reference"] = result.pop("repair")
        result["candidate"] = result.pop("history")
        result["both_match_original_physical_state"] = sum(r["both_match_original_physical_state"] for r in rows)
        return result

    report = {"reference_run": args.reference.name, "candidate_run": args.candidate.name,
              "scope": "State-change counters are relative to the common original control. Both-zero checks establish mutual physical equality.",
              "all": summarize(paired), "by": {}}
    for field in ("opponent", "branch", "panel"):
        report["by"][field] = {str(value): summarize([r for r in paired if r[field] == value])
                               for value in sorted({r[field] for r in paired})}
    (args.candidate / "VS_REFERENCE.json").write_text(json.dumps(report, indent=2) + "\n")
    exported = [{k: v for k, v in r.items() if k not in ("repair", "history")} for r in paired]
    (args.candidate / "VS_REFERENCE_PAIRED.json").write_text(json.dumps(exported, indent=2) + "\n")
    print(json.dumps(report["all"], indent=2))


if __name__ == "__main__":
    main()
