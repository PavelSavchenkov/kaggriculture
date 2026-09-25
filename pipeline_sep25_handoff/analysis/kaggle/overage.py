"""Our overage time in Kaggle replays: start, end, steps that used overage, largest use (day:hour)."""
import csv, gzip, json, sys
sub = sys.argv[1]
for row in csv.DictReader(open(f'meta_{sub}.csv')):
    r = json.load(gzip.open(f"replays/{row['episode']}.json.gz"))
    seat = int(row['seat'])
    ov = [st[seat]['observation'].get('remainingOverageTime') for st in r['steps']]
    ov = [x for x in ov if x is not None]
    uses = [(ov[i - 1] - ov[i], i - 1) for i in range(1, len(ov)) if ov[i] < ov[i - 1] - 1e-9]
    big = sorted(uses, reverse=True)[:3]
    opp = [st[1 - seat]['observation'].get('remainingOverageTime') for st in r['steps']]
    print(row['episode'], f"start {ov[0]:.1f} end {ov[-1]:.1f} steps_using {len(uses)} top",
          ' '.join(f"{u:.2f}s@d{(s)//24}h{(s)%24}" for u, s in big), f"| opp end {opp[-1]}")
