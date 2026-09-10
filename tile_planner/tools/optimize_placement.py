"""Bounded orchestration of the C++ placement, point-solver and compiler tools.

Input is a complete compile_cold course with recorded context. Expensive search,
biological compilation, routing, bank reuse and verification remain in C++.
"""
import argparse
import csv
import hashlib
import json
import random
import shutil
import subprocess
import time
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("incumbent", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--seconds", type=float, default=180)
    parser.add_argument("--mode", choices=["search", "fixed"], default="search")
    parser.add_argument("--seed", type=int, default=20260910)
    parser.add_argument("--point-seconds", type=float, default=6)
    parser.add_argument("--objective", choices=["hire", "cash", "margin"], default="hire")
    parser.add_argument("--tools", type=Path, default=EXP / "build")
    args = parser.parse_args()
    if args.seconds < 0 or args.point_seconds <= 0:
        raise ValueError("invalid budget")
    source = args.incumbent.resolve()
    original = json.loads((source / "SUMMARY.json").read_text())
    if not original["complete"] or "seed" not in original or "weed_chance" not in original:
        raise ValueError("input must be a complete course with recorded seed and weed chance")
    if original.get("conditional_placement"):
        raise ValueError("this search takes a fixed lifetime assignment; conditional policies need a separate outer comparison")
    began = time.monotonic()
    output = args.output.resolve(); output.mkdir(exist_ok=False)
    copied = output / "incumbent"
    shutil.copytree(source, copied)
    if original.get("finance_source") and not (copied / "finance_source.txt").exists():
        (copied / "finance_source.txt").write_text(original["finance_source"] + "\n")
    tools = ["probe_lives", "solve_point", "pack_bank", "compile_cold", "export_calendar"]
    for name in tools:
        shutil.copy2(args.tools / name, output / name)
    protocol = {"input": str(source), "seconds": args.seconds, "mode": args.mode, "search_seed": args.seed,
        "point_seconds": args.point_seconds, "objective": args.objective, "workers": 1, "maximum_fixed_points": 12,
        "proposal_order": "family round robin, best two predictions then uniform exploration; randomized prediction ties",
        "initial_refinement": "both modes: 0.1-second warm-day refinement, then one base-budget lower-workforce query on each of the two most expensive days",
        "unused_search_budget": "after the finite placement queue is exhausted, refine the best complete assignment with increasing point budgets",
        "acceptance": "complete C++ engine run and independent financial-calendar verification; keep the incumbent on failure or timeout",
        "binary_sha256": {name: hashlib.sha256((output / name).read_bytes()).hexdigest() for name in tools}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    events = []
    best_path, best = copied, original
    complete = [{"path": "incumbent", "hire_bill": original["hire_bill"], "cash": original["cash"], "opponent_cash": original["opponent_cash"]}]
    bank_paths = [str(copied)]
    bank_version = 0
    input_reverified = False
    input_rejected = False
    termination_reason = "budget_or_search_exhausted"

    def remaining():
        return max(0, args.seconds - (time.monotonic() - began))

    def key(row):
        if args.objective == "cash":
            return -row["cash"], row["hire_bill"]
        if args.objective == "margin":
            return row["opponent_cash"] - row["cash"], row["hire_bill"]
        return row["hire_bill"], -row["cash"]

    def record():
        report = {"status": "invalid_incumbent" if input_rejected else "complete_incumbent_retained", "best": None if input_rejected else str(best_path.relative_to(output)),
            "hire_bill": best["hire_bill"], "cash": best["cash"], "opponent_cash": best["opponent_cash"],
            "original_hire_bill": original["hire_bill"], "original_cash": original["cash"],
            "cash_change": best["cash"] - original["cash"], "objective": args.objective,
            "improved": key(best) < key(original), "elapsed_seconds": time.monotonic() - began,
            "budget_seconds": args.seconds, "mode": args.mode, "events": events,
            "input_reverified": input_reverified, "termination_reason": termination_reason,
            "complete_candidates": [] if input_rejected else complete,
            "pareto_candidates": [] if input_rejected else [row for row in complete if not any(other["hire_bill"] <= row["hire_bill"] and other["cash"] >= row["cash"]
                and (other["hire_bill"] < row["hire_bill"] or other["cash"] > row["cash"]) for other in complete)]}
        temporary = output / "RESULT.tmp"
        temporary.write_text(json.dumps(report, indent=2) + "\n")
        temporary.replace(output / "RESULT.json")

    def run(name, arguments, label, cap=None):
        allowance = remaining() if cap is None else min(remaining(), cap)
        if allowance <= 0:
            return False
        start = time.monotonic()
        with (output / (label + ".log")).open("w") as stream:
            try:
                result = subprocess.run([str(output / name), *map(str, arguments)], stdout=stream, stderr=subprocess.STDOUT, timeout=allowance)
                status = result.returncode
            except subprocess.TimeoutExpired:
                status = "timeout"
        events.append({"tool": name, "label": label, "status": status, "seconds": time.monotonic() - start})
        record()
        return status == 0

    def pack():
        nonlocal bank_version
        manifest = output / f"bank_{bank_version}.txt"
        manifest.write_text("\n".join(bank_paths) + "\n")
        bank = output / f"bank_{bank_version}.bin"
        bank_version += 1
        return bank if run("pack_bank", [manifest, bank, output / "validate.txt"], bank.stem, 3) else None

    def compile_candidate(assignment, label, bank, warm_seconds=0):
        nonlocal best, best_path
        folder = output / label
        options = ["--bank", bank, "--warm_seconds", str(warm_seconds), "--opponent", original["opponent"].removeprefix("live_"),
            "--seat", original["seat"], "--finance", original["finance"]]
        if original.get("shop_seed") is not None:
            options += ["--shop_seed", original["shop_seed"]]
        if original.get("finance_source"):
            options += ["--finance_source", original["finance_source"]]
        if original.get("preparation_repair_enabled"):
            options += ["--repair_preparation", "1"]
        if original.get("opponent_trace"):
            options += ["--opponent_trace", original["opponent_trace"]]
        success = run("compile_cold", [source / "INPUT.plan", folder, assignment, 9, 3, original["seed"], original["weed_chance"], *options], label)
        if not success:
            return False
        row = json.loads((folder / "SUMMARY.json").read_text())
        if not run("export_calendar", [folder, original["seed"], original["weed_chance"], original["seat"], folder / "calendar"], label + "_verify", 2):
            return False
        bank_paths.append(str(folder))
        complete.append({"path": label, "hire_bill": row["hire_bill"], "cash": row["cash"], "opponent_cash": row["opponent_cash"]})
        if key(row) < key(best):
            best, best_path = row, folder
        record()
        return True

    record()
    if remaining() == 0:
        termination_reason = "budget_before_input_recheck"
        record()
        return
    # This rechecks the given realized course; it does not use future events
    # to propose placements. The supplied incumbent remains available.
    if not run("export_calendar", [copied, original["seed"], original["weed_chance"], original["seat"], output / "input_check"], "input_check", 3):
        if remaining() == 0 or (events and events[-1]["status"] == "timeout"):
            termination_reason = "budget_during_input_recheck"
            record()
            return
        input_rejected = True
        termination_reason = "incumbent_verification_failed"
        record()
        raise RuntimeError("incumbent verification failed")
    input_reverified = True
    record()
    (output / "validate.txt").write_text(str(copied) + "\n")
    bank = pack()
    if bank is None:
        return
    daily = list(csv.DictReader((copied / "days.csv").open()))
    workforce = {int(row["day"]): int(row["workers"]) for row in daily}
    peaks = sorted(workforce, key=lambda day: (-workforce[day], day))[:2]

    compile_candidate(copied / "assignment.txt", "initial_micro", bank, 0.1)
    bank = pack()
    workforce = {int(row["day"]): int(row["workers"]) for row in csv.DictReader((best_path / "days.csv").open())}
    peaks = sorted(workforce, key=lambda day: (-workforce[day], day))[:2]
    for day in peaks:
        if bank is None or remaining() <= 2 or workforce[day] <= 1:
            break
        folder = output / "points" / f"initial_{day}" / f"{day:02d}"
        seconds = min(args.point_seconds, max(0.01, remaining() - 1))
        success = run("solve_point", [best_path / f"{day:02d}", day, workforce[day] - 1, seconds, folder], f"initial_point_{day}")
        if success and json.loads((folder / "SUMMARY.json").read_text())["status"] == "feasible":
            bank_paths.append(str(folder.parent)); bank = pack()
            if bank is not None:
                compile_candidate(copied / "assignment.txt", f"initial_complete_{day}", bank)
    if bank is None:
        return
    workforce = {int(row["day"]): int(row["workers"]) for row in csv.DictReader((best_path / "days.csv").open())}
    day_bill = {int(row["day"]): int(row["bill"]) for row in csv.DictReader((best_path / "days.csv").open())}
    peaks = sorted(workforce, key=lambda day: (-workforce[day], day))[:2]

    def refine_remaining(first_attempt, prefix):
        nonlocal bank
        current_workers = {int(row["day"]): int(row["workers"]) for row in csv.DictReader((best_path / "days.csv").open())}
        current_peaks = sorted(current_workers, key=lambda day: (-current_workers[day], day))[:2]
        attempt = first_attempt
        while bank is not None and remaining() > 2 and attempt < 12:
            day = current_peaks[attempt % len(current_peaks)]
            seconds = min(args.point_seconds * 2 ** (attempt // len(current_peaks)), max(0.01, remaining() - 1))
            folder = output / "points" / f"{prefix}_{attempt}" / f"{day:02d}"
            if current_workers[day] <= 1:
                break
            success = run("solve_point", [best_path / f"{day:02d}", day, current_workers[day] - 1, seconds, folder], f"{prefix}_point_{attempt}")
            if success and json.loads((folder / "SUMMARY.json").read_text())["status"] == "feasible":
                bank_paths.append(str(folder.parent)); bank = pack()
                if bank is not None:
                    compile_candidate(best_path / "assignment.txt", f"{prefix}_complete_{attempt}", bank)
                    current_workers = {int(row["day"]): int(row["workers"]) for row in csv.DictReader((best_path / "days.csv").open())}
            attempt += 1
        record()

    if args.mode == "fixed":
        refine_remaining(2, "fixed")
        return

    proposals = output / "proposals"
    if not run("probe_lives", [best_path, proposals, args.seed], "proposals", min(30, max(1, args.seconds / 4))):
        refine_remaining(0, "fallback")
        return
    metadata = json.loads((proposals / "SUMMARY.json").read_text()); peaks = metadata["peak_days"]
    rows = [row for row in csv.DictReader((proposals / "probes.csv").open()) if row["selected"] == "1" and row["family"] != "control"]
    rng = random.Random(args.seed); groups = {}
    for row in rows:
        groups.setdefault(row["family"], []).append(row)
    for group in groups.values():
        rng.shuffle(group)
        group.sort(key=lambda row: float(row["peak0_cost"]) + float(row["peak1_cost"]))
        tail = group[2:]; rng.shuffle(tail); group[2:] = tail
    families = [name for name in ["crop_chain", "animal_swap", "animal_move", "quadrant_counts"] if name in groups]
    queue = []
    for index in range(max(map(len, groups.values()), default=0)):
        for family in families:
            if index < len(groups[family]):
                queue.append(groups[family][index])
    for attempt, row in enumerate(queue):
        if remaining() <= 2:
            break
        affected = [i for i, day in enumerate(peaks) if int(row["affected_mask"]) & (1 << day) and workforce[day] > 1]
        if not affected:
            continue
        index = min(affected, key=lambda i: float(row[f"peak{i}_cost"]) / max(1, day_bill[peaks[i]]))
        day = peaks[index]
        point = output / "points" / row["id"] / f"{day:02d}"
        seconds = min(args.point_seconds, max(0.01, remaining() - 1))
        success = run("solve_point", [proposals / row["id"] / f"{day:02d}", day, workforce[day] - 1, seconds, point], f"point_{row['id']}")
        if not success or json.loads((point / "SUMMARY.json").read_text())["status"] != "feasible":
            continue
        bank_paths.append(str(point.parent)); bank = pack()
        if bank is None:
            break
        compile_candidate(proposals / row["id"] / "assignment.txt", f"candidate_{row['id']}", bank)
        # Any new route witness is also offered to the unchanged layout. This
        # distinguishes geometry gains from transferable scheduling gains.
        if remaining() > 2:
            bank = pack()
            if bank is not None:
                compile_candidate(copied / "assignment.txt", f"fixed_reuse_{attempt}", bank)
    refine_remaining(0, "fallback")
    record()


if __name__ == "__main__":
    main()
