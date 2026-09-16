"""Regenerate the unrestricted benchmark report from archived per-call rows."""
import argparse
import csv
import io
import json
import statistics
import zipfile
from pathlib import Path


FEATURES = ("late_inputs", "fertilizer_buys", "fertilizer_pickups", "late_hires")
CONFIGS = {
    "cap10": {"label": "Balanced-4 cap 10", "cap": 10},
    "cap11": {"label": "Balanced-4 cap 11", "cap": 11},
    "cap13_fixed": {"label": "Balanced-4 cap 13", "cap": 13},
    "unrestricted": {"label": "unrestricted_day_policy", "cap": 13},
}


def read(archive, name):
    with archive.open(name) as raw:
        return list(csv.DictReader(io.TextIOWrapper(raw, newline="")))


def coverage(rows):
    solved = sum(row["status"] == "0" for row in rows)
    return {"solved": solved, "days": len(rows), "coverage": solved / len(rows)}


def timing(rows_a, rows_b):
    values = [float(row["microseconds"]) / 1000 for rows in (rows_a, rows_b) for row in rows]
    return {"median_ms": statistics.median(values), "mean_ms": statistics.fmean(values)}


def summarize(rows_a, rows_b, cap):
    late = [row for row in rows_a if 20 <= int(row["day"]) <= 28]
    solved = [row for row in rows_a if row["status"] == "0"]
    value = coverage(rows_a) | timing(rows_a, rows_b)
    value["late"] = coverage(late)
    value["mean_hires"] = statistics.fmean(int(row["hires"]) for row in solved)
    value["hires_saved_from_cap"] = sum(cap - int(row["hires"]) for row in solved)
    return value


def ratio(value):
    return f"{value['solved']:,}/{value['days']:,} ({value['coverage']:.1%})"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = {"method": "Two sequential CPU14-pinned native Release runs; failed calls included.", "cohorts": {}}
    with zipfile.ZipFile(args.archive) as archive:
        metadata = {}
        for cohort in ("dev", "valid"):
            metadata[cohort] = {
                (row["game"], row["seat"], row["day"]): row
                for row in read(archive, f"{cohort}_cases.csv") if row["reason"] == "eligible"
            }
            report["cohorts"][cohort] = {}
            for name, config in CONFIGS.items():
                rows_a = read(archive, f"{cohort}_{name}_a.csv")
                rows_b = read(archive, f"{cohort}_{name}_b.csv")
                fields = [field for field in rows_a[0] if field != "microseconds"]
                assert [[row[field] for field in fields] for row in rows_a] == [
                    [row[field] for field in fields] for row in rows_b
                ]
                chosen = [
                    index for index, row in enumerate(rows_a)
                    if int(metadata[cohort][(row["game"], row["seat"], row["day"])]["original_hires"])
                    <= config["cap"]
                ]
                filtered_a = [rows_a[index] for index in chosen]
                filtered_b = [rows_b[index] for index in chosen]
                value = summarize(filtered_a, filtered_b, config["cap"])
                value["original_hires_at_most"] = config["cap"]
                value["all_eligible_stress"] = summarize(rows_a, rows_b, config["cap"])
                value["repeat_non_time_parity"] = True
                report["cohorts"][cohort][name] = value
        for cohort in ("dev", "valid"):
            rows = read(archive, f"{cohort}_unrestricted_a.csv")
            feature_report = {}
            for feature in (*FEATURES, "any_removed_restriction"):
                chosen = []
                for row in rows:
                    meta = metadata[cohort][(row["game"], row["seat"], row["day"])]
                    present = any(int(meta[item]) for item in FEATURES) if feature == "any_removed_restriction" else int(meta[feature]) > 0
                    if present:
                        chosen.append(row)
                feature_report[feature] = coverage(chosen)
            report["cohorts"][cohort]["restriction_features"] = feature_report
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    lines = [
        "# Unrestricted day-policy measurements", "",
        report["method"] + " Late means days 20-28. Each primary row includes only days where the "
        "original schedule used no more hires than that row's cap.", "",
        "| Cohort | Profile / cap | All days | Late days | Median / mean ms | Mean hires |",
        "|---|---|---:|---:|---:|---:|",
    ]
    for cohort in ("dev", "valid"):
        for name, config in CONFIGS.items():
            value = report["cohorts"][cohort][name]
            lines.append(f"| {cohort} | {config['label']} | {ratio(value)} | {ratio(value['late'])} | "
                         f"{value['median_ms']:.2f} / {value['mean_ms']:.2f} | {value['mean_hires']:.2f} |")
    dev_saved = report["cohorts"]["dev"]["unrestricted"]["hires_saved_from_cap"]
    valid_saved = report["cohorts"]["valid"]["unrestricted"]["hires_saved_from_cap"]
    lines.extend(["", "The production unrestricted profile preserves cap-13 coverage while saving "
                  f"{dev_saved:,} hires on dev and {valid_saved:,} on validation.", "",
                  "## All-expanded-set stress test", "",
                  "These rows deliberately ignore original hire count and are not the cap-coverage figures above.", "",
                  "| Cohort | Profile / cap | All days | Late days |", "|---|---|---:|---:|"])
    for cohort in ("dev", "valid"):
        for name, config in CONFIGS.items():
            value = report["cohorts"][cohort][name]["all_eligible_stress"]
            lines.append(f"| {cohort} | {config['label']} | {ratio(value)} | {ratio(value['late'])} |")
    lines.extend(["",
                  "## Removed-restriction cases", "",
                  "| Cohort | Late seed/animal buy | Fertilizer buy | Fertilizer pickup | Hire after hour 1 | Any |",
                  "|---|---:|---:|---:|---:|---:|"])
    for cohort in ("dev", "valid"):
        value = report["cohorts"][cohort]["restriction_features"]
        lines.append(f"| {cohort} | {ratio(value['late_inputs'])} | {ratio(value['fertilizer_buys'])} | "
                     f"{ratio(value['fertilizer_pickups'])} | {ratio(value['late_hires'])} | "
                     f"{ratio(value['any_removed_restriction'])} |")
    (args.output / "REPORT.md").write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
