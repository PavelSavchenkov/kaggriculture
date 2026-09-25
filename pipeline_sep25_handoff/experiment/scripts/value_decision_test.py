"""Decision test of the value model (work/sep25_bc_weakness/VALUE_MODEL.md step 5.3). At every
searched dawn of tools/search_games run with SEARCH_ROLLOUTS=<dir>, each candidate was played to
the end with exact copies of both agents (final margin) and saved as a trace. Here each
candidate's next-dawn state is extracted and scored by V; the V ranking is compared with the
rollout ranking: top-1 agreement, Spearman correlation, regret of argmax V (rollout-best margin
minus the margin of V's choice) against the regret of the baseline (candidate 0).

usage: value_decision_test.py --model models/value_v1 reports/value_test/king_rc4 [...]
"""
import argparse
import csv
import subprocess
from pathlib import Path

import numpy as np
import torch

import train as bc
import train_value as tv

ROOT = Path(__file__).resolve().parents[1]


def extract(directory, build):
    rows = list(csv.reader(open(directory / "rollouts/rollouts.csv")))
    keys = {}
    with open(directory / "corpus.txt", "w") as corpus:
        for episode, (seed, seat, day, cand, margin) in enumerate(rows):
            trace = directory / f"rollouts/{seed}_{seat}_{day}_{cand}.txt"
            corpus.write(f"{episode} {seat} 0 0 validation {trace}\n")
            keys[episode] = (int(seed), int(seat), int(day), int(cand), float(margin))
    arrays = directory / "arrays"
    if not (arrays / "shard_00.meta.i64").exists():
        arrays.mkdir(exist_ok=True)
        subprocess.run([str(ROOT / build / "extract"), str(directory / "corpus.txt"), "0", "1", str(arrays / "shard_00")],
                       check=False, stdout=subprocess.DEVNULL)
    return keys


def main():
    p = argparse.ArgumentParser()
    p.add_argument("dirs", nargs="+", type=Path)
    p.add_argument("--model", default="models/value_v1")
    p.add_argument("--build", default="build_vt")
    p.add_argument("--width", type=int, default=256)
    args = p.parse_args()
    device = "cuda" if torch.cuda.is_available() else "cpu"
    net = tv.ValueNet(args.width).to(device)
    net.load_state_dict(torch.load(ROOT / args.model / "value.pt", map_location=device))
    net.eval()
    groups = {}  # (dir, seed, seat, day) -> {cand: (margin, value)}
    for directory in args.dirs:
        keys = extract(directory, args.build)
        d = bc.load(str(directory / "arrays"))
        bc.Batch.conditions = {(e, k[1]): (150.0, 40.0) for e, k in keys.items()}
        bc.Batch.condition_rows = bc.condition_table(d, bc.Batch.conditions)
        meta = d["meta"]
        rows = np.array([i for i, (e, seat, day) in enumerate(meta[:, :3])
                         if int(e) in keys and keys[int(e)][1] == seat and keys[int(e)][2] + 1 == day])
        with torch.no_grad():
            value = np.concatenate([net(bc.Batch(d, rows[i:i + 2048], device))[0].cpu().numpy() * tv.SCALE
                                    for i in range(0, len(rows), 2048)])
        for r, v in zip(rows, value):
            seed, seat, day, cand, margin = keys[int(meta[r, 0])]
            groups.setdefault((str(directory), seed, seat, day), {})[cand] = (margin, float(v))
    stats = {"groups": 0, "top1": 0, "rho": [], "regret_v": [], "regret_base": [], "regret_random": [], "gain_v": []}
    seen = set()
    for key, cands in sorted(groups.items()):
        if len(cands) < 3 or 0 not in cands:
            continue
        m = np.array([cands[c][0] for c in sorted(cands)])
        v = np.array([cands[c][1] for c in sorted(cands)])
        base = sorted(cands).index(0)
        signature = (key[0], key[1], key[3], tuple(np.round(m)))
        if m.max() - m.min() < 1 or signature in seen:  # no decision, or the mirrored seat
            continue
        seen.add(signature)
        stats["groups"] += 1
        stats["top1"] += int(m[np.argmax(v)] == m.max())
        rm, rv = np.argsort(np.argsort(m)), np.argsort(np.argsort(v))
        stats["rho"].append(np.corrcoef(rm, rv)[0, 1])
        stats["regret_v"].append(m.max() - m[np.argmax(v)])
        stats["regret_base"].append(m.max() - m[base])
        stats["regret_random"].append(m.max() - m.mean())
        stats["gain_v"].append(m[np.argmax(v)] - m[base])
    n = stats["groups"]
    print(f"decision dawns {n} (candidates differ; mirrored seats removed)")
    print(f"top-1 agreement {stats['top1'] / n:.2f} (random {1 / 7:.2f}); Spearman mean {np.mean(stats['rho']):.3f}")
    for k in ("regret_v", "regret_base", "regret_random"):
        print(f"{k}: mean {np.mean(stats[k]):.0f}")
    g = np.array(stats["gain_v"])
    se = g.std() / np.sqrt(len(g))
    print(f"argmax V vs baseline: mean {g.mean():+.0f} (+-{1.96 * se:.0f}), better {np.mean(g > 0):.2f} worse {np.mean(g < 0):.2f}")


if __name__ == "__main__":
    main()
