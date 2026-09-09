"""Prepare ordinary-day marginal diagnostics on exposed complete courses."""
import argparse
import hashlib
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--additional-course", type=Path, nargs="*", default=[])
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    source = EXP / "data/warm_courses_v1"
    courses = [(row["name"], source / row["name"]) for row in json.loads((source / "MANIFEST.json").read_text())["courses"]
               if row["status"]["status"] == "compiled"]
    courses += [(p.parent.name, p.resolve()) for p in args.additional_course]
    inputs, pairs, unsupported = {}, [], []

    def add(path):
        sha = hashlib.sha256(path.read_bytes()).hexdigest()
        inputs[sha[:20]] = str(path)
        return sha[:20]

    for name, root in courses:
        assert json.loads((root / "STATUS.json").read_text())["status"] == "compiled"
        for day in sorted((root / "days").iterdir(), key=lambda p: int(p.name)):
            if day.name == "29":
                unsupported.append({"course": name, "day": 29, "reason": "frozen model supports only 24 real phases"})
                continue
            candidate = day / "problem.json"
            baseline = EXP / "runs/warm_source_v1" / day.name / "problem.json"
            assert candidate.is_file() and baseline.is_file()
            a, b = json.loads(baseline.read_text()), json.loads(candidate.read_text())
            pairs.append({"course": name, "day": int(day.name), "baseline": add(baseline), "candidate": add(candidate),
                "baseline_observed_workers": a["worker_count"], "candidate_observed_workers": b["worker_count"],
                "baseline_tasks": sum(len(w["actions"]) for w in a["tile_work"]),
                "candidate_tasks": sum(len(w["actions"]) for w in b["tile_work"]),
                "baseline_hire_hours": [e["hour"] for e in a["buy_schedule"] if e["op"] == "hire"],
                "candidate_hire_hours": [e["hour"] for e in b["buy_schedule"] if e["op"] == "hire"]})
    (output / "INPUT.txt").write_text("".join(f"{key} {path}\n" for key, path in sorted(inputs.items())))
    (output / "WITNESSES.txt").write_text("".join(f"{key} {path} {Path(path).with_name('actions.txt')}\n" for key, path in sorted(inputs.items())))
    report = {"scope": "Exposed whole-course examples; candidate routes and observed workers are evidence only, never model inputs. The normalized earliest-hire model is being diagnosed against inherited-calendar bills, a target mismatch.",
              "pairs": pairs, "unsupported": unsupported}
    (output / "PAIRS.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(pairs)} ordinary-day pairs, {len(inputs)} input variants, {len(unsupported)} terminal days explicitly unsupported.")


if __name__ == "__main__":
    main()
