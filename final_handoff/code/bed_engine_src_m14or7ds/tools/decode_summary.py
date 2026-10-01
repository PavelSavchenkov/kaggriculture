"""Summarize decode_eval CSVs: per-field predicted vs label totals and errors, by day range."""
import csv
import sys

FIELDS = ["new_crop", "new_animal", "harvest", "retain", "clear", "feed", "care", "collect", "water", "fert"]
for path in sys.argv[1:]:
    rows = list(csv.DictReader(open(path)))
    print(path, len(rows), "dawns")
    for lo, hi in [(0, 9), (10, 19), (20, 28), (29, 29)]:
        r = [x for x in rows if lo <= int(x["day"]) <= hi]
        parts = []
        for f in FIELDS:
            p = sum(int(x[f + "_pred"]) for x in r)
            l = sum(int(x[f + "_label"]) for x in r)
            parts.append(f"{f} {p / len(r):.1f}/{l / len(r):.1f}")
        land = sum(x["land_pred"] != x["land_label"] for x in r)
        members = sum(int(x["oneshot_members"]) for x in r)
        mism = sum(int(x["oneshot_mismatch"]) for x in r)
        crop_abs = sum(int(x["new_crop_abs"]) for x in r) / len(r)
        print(f"  days {lo}-{hi}: " + " ".join(parts) + f" | new_crop MAE {crop_abs:.1f} land errors {land}"
              f" one-shot member mismatch {mism / max(members, 1):.3f}")
