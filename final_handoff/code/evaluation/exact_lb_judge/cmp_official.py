import glob, json, sys
off = {(g['opponent'], g['seed'], g['seat']): g for g in json.load(open(sys.argv[2]))}
ok = n = 0
for f in sorted(glob.glob(f'{sys.argv[1]}/*.json')):
    r = json.load(open(f)); o = off.get((r['opponent'], r['seed'], r['seat']))
    if o is None: continue
    n += 1; same = [round(x) for x in r['money']] == [round(x) for x in o['money']]; ok += same
    print(r['opponent'][:30], r['seed'], r['seat'], 'replay', r['money'], 'official', o['money'], 'OK' if same else 'DIFF', f"{r['wall']:.0f}s", (r['error'] or '')[:80])
print(f'identity {ok} / {n}')
