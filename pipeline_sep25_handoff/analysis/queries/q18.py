import re, collections
fib=[1,1,2,3,5,8,13,21,34,55,89,144,233,377,610]
wage=lambda n: sum(fib[:n])
rows=[]
for sd in (701,702,703,704,705):
    day=None; cur=collections.defaultdict(list); done=False
    for line in open(f'/tmp/claude-1000/-home-pavel-Programming-kaggriculture/cf94136b-def3-409b-aeb3-b1395026af5b/scratchpad/dbg{sd}.txt'):
        m=re.match(r'd(\d\d) ',line)
        if m:
            day=int(m.group(1))
            if day==29: break
            continue
        m=re.search(r'return L(\d) F(\d) carried (\d+) room (\d+) wanted (\d+) percent (\d+) due (\d+) status (\d+).*hires (\d+) \(base (\d+)',line)
        if m:
            d=0 if day is None else day+1
            cur[d].append(tuple(map(int,m.groups())))
    for d,att in cur.items():
        ok=[a for a in att if a[7]==0]
        base=att[-1][9]
        final=ok[-1][8] if ok else base
        rows.append((sd,d,base,final,att[-1][2],att[-1][4]))
import statistics as st
mid=[r for r in rows if 10<=r[1]<=28]
print('days 10-28, 5 games:', len(mid), 'days')
print('mean base hires %.2f, final %.2f' % (st.mean(r[2] for r in mid), st.mean(r[3] for r in mid)))
print('mean wage base $%.0f final $%.0f' % (st.mean(wage(r[2]) for r in mid), st.mean(wage(r[3]) for r in mid)))
print('base hire distribution', sorted(collections.Counter(r[2] for r in mid).items()))
print('final hire distribution', sorted(collections.Counter(r[3] for r in mid).items()))
print('mean carried at end of base route %.0f, wanted (market part) %.0f' % (st.mean(r[4] for r in mid), st.mean(r[5] for r in mid)))
