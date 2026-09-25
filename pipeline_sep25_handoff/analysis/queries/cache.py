"""Caches per-perspective (g) and per-day (d) tables plus animals/crops as pickles."""
import sys
import pandas as pd
from analyze import load, perspectives
from compare import source
parts = [load(p) for p in sys.argv[2:]]
data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in parts[0]}
g, d = perspectives(data, source)
key = g[["trace", "seat", "source"]]
pd.to_pickle(dict(g=g, d=d, animals=data["animals"].merge(key, on=["trace", "seat"]),
                  crops=data["crops"].merge(key, on=["trace", "seat"])), sys.argv[1])
print(g.source.value_counts())
