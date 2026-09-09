"""Record the two corrected witness metadata cases without changing frozen data."""
import csv
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    original = EXP / "data/development_round1_days/MANIFEST.json"
    manifest = json.loads(original.read_text())
    game = next(g for g in manifest["games"] if g["episode"] == 106958418 and g["seat"] == 1)
    fixed = EXP / "runs/replay_extract_mengfei_fixed"
    outcomes = {int(r["day"]): r for r in csv.DictReader((fixed / "days.csv").open())}
    amendments = []
    for day in [1, 9]:
        before = next(r for r in game["days"] if r["day"] == day)
        after = outcomes[day]
        assert before["status"] == "SOURCE_REJECTED" and after["status"] == "FEASIBLE_SOURCE"
        problem = fixed / f"{day:02}/problem.json"
        witness = fixed / f"{day:02}/physical_source.actions.txt"
        assert json.loads(problem.read_text()) == json.loads((EXP / before["problem"]).read_text())
        amendments.append({"episode": game["episode"], "seat": 1, "day": day, "before": before,
                           "after": after, "problem": str(problem.relative_to(EXP)), "problem_sha256": sha(problem),
                           "witness": str(witness.relative_to(EXP)), "witness_sha256": sha(witness)})
    record = {"utc": datetime.now(timezone.utc).isoformat(), "original_manifest_sha256": sha(original),
              "extractor_sha256": sha(EXP / "include/extract_contract.hpp"),
              "fix": "Set sanitized action n_units to the engine-accepted count.",
              "effect": "899 ordinary days now have strict source witnesses. Frozen v0 index and reference jobs still use the original 897.",
              "amendments": amendments}
    (EXP / "evidence/extraction_amendment_v1.json").write_text(json.dumps(record, indent=2) + "\n")
    print(record["effect"])


if __name__ == "__main__":
    main()
