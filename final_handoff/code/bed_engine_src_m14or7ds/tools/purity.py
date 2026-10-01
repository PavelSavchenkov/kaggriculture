"""How often are expert group decisions all-or-none (pure) versus partial?"""
import sys
import numpy as np
sys.path.insert(0, "scripts")
from train import load, CROP_ONGOING

d = load(sys.argv[1])
ct, cm, c = d["ctarget"].astype(int), d["cmask"].astype(bool), d["crop"]
at, am = d["atarget"].astype(int), d["amask"].astype(bool)
size = ct[:, 0]
ongoing = c[:, CROP_ONGOING] > 0.5
multi = size >= 2
one = (~ongoing) & multi
pure = (ct[one, 1:10].max(1) == size[one])
print(f"one-shot groups (size>=2): {one.sum()}, one option for all members: {pure.mean():.3f}")
for name, k, n, mask in [("harvest", ct[:, 13], size, cm[:, 13] | ~ongoing), ("retain", ct[:, 10], size, cm[:, 9] | ~ongoing)]:
    ok = (~mask) & (n >= 2)
    p = (k[ok] == 0) | (k[ok] == n[ok])
    print(f"ongoing {name} (size>=2): {ok.sum()}, all-or-none {p.mean():.3f}")
asize = at[:, 0]
for name, j, base, mask in [("feed", 1, asize, am[:, 0]), ("care", 2, at[:, 1], am[:, 1]), ("collect", 3, asize, am[:, 2])]:
    ok = (~mask) & (base >= 2)
    k = at[ok, j]
    p = (k == 0) | (k == base[ok])
    print(f"animal {name} (n>=2): {ok.sum()}, all-or-none {p.mean():.3f}, mean fraction {np.mean(k / base[ok]):.3f}")
