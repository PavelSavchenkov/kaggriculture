"""Concatenate CSV shards with one header: merge_csv.py out.csv shard1.csv ..."""
import csv
import sys

rows = []
for path in sys.argv[2:]:
    rows += list(csv.DictReader(open(path)))
with open(sys.argv[1], 'w', newline='') as out:
    writer = csv.DictWriter(out, fieldnames=list(rows[0].keys()))
    writer.writeheader()
    writer.writerows(rows)
