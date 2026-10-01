"""Value model V(dawn state): expected final result of the game from a dawn, from replay dawns
(the BC arrays) labelled with each perspective's final margin (data/perspectives_meta.csv; replay
DB rows from data/replay_db/perspectives.csv). Same encoders as the BC network (scripts/train.py);
heads: margin / 20,000 (Huber) and win (logistic). Style inputs are off; strength/recency
conditioning is kept (a stronger player's state is worth more).

Reports per game day on held-out episodes: R^2 of the margin head and win accuracy, against a
ridge baseline on the same 256 dawn features.

--td <model dir>: TD(lambda) targets from that value model instead of Monte Carlo ones:
y_d = (1 - lambda) V(s_next) + lambda y_next along each perspective, y_last = final margin (less
label noise on early days). --gpu-data keeps the arrays on the GPU (train.py GpuData).

--margins <csv>: extra labels (episode, seat, margin), e.g. on-policy games (build_value_corpus.py);
--init <model dir>: start from that value model (fine-tuning).

usage: train_value.py --arrays data/arrays_v5 --out models/value_v1 [--steps 20000] [--td models/value_v1]
"""
import argparse
import csv
import json
import time
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F

import train as bc

SCALE = 20000.0


class ValueNet(nn.Module):
    def __init__(self, w=256):
        super().__init__()
        self.g_enc, self.c_enc, self.a_enc = bc.mlp(bc.GLOBAL, w, w), bc.mlp(bc.CROP, w, w), bc.mlp(bc.ANIMAL, w, w)
        self.grid = bc.GridNet()
        self.ctx = bc.mlp(7 * w + 128, w, w)
        self.head = nn.Linear(w, 2)  # margin / SCALE, win logit

    def forward(self, b):
        g = F.relu(self.g_enc(b.g))
        c = F.relu(self.c_enc(b.c))
        a = F.relu(self.a_enc(b.a))
        keep_c = 1.0 - b.c[:, bc.CROP_FRESH]
        keep_a = 1.0 - b.a[:, bc.ANIMAL_FRESH]
        parts = [g, bc.pool(c, b.ci, keep_c, b.n), bc.pool(a, b.ai, keep_a, b.n),
                 self.grid(b.grid[:, 0]), self.grid(b.grid[:, 1])]
        out = self.head(F.relu(self.ctx(torch.cat(parts, 1))))
        return out[:, 0], out[:, 1]


def results():
    """(episode, seat) -> final margin."""
    out = {}
    for r in csv.DictReader(open(bc_root() / "data/perspectives_meta.csv")):
        if r["margin"]:
            out[(int(r["episode"]), int(r["seat"]))] = float(r["margin"])
    path = bc_root() / "data/replay_db/perspectives.csv"
    if path.exists():
        reward = {(int(r["episode"]), int(r["seat"])): float(r["reward"]) for r in csv.DictReader(open(path))}
        for (e, s), v in reward.items():
            if (e, 1 - s) in reward:
                out.setdefault((e, s), v - reward[(e, 1 - s)])
    return out


def bc_root():
    return Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--arrays", default="data/arrays_v5")
    p.add_argument("--out", required=True)
    p.add_argument("--steps", type=int, default=20000)
    p.add_argument("--batch", type=int, default=1024)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--width", type=int, default=256)
    p.add_argument("--condition", default="data/conditions_v5.csv")
    p.add_argument("--td", default=None)
    p.add_argument("--lam", type=float, default=0.8)
    p.add_argument("--gpu-data", action="store_true")
    p.add_argument("--margins", default=None)
    p.add_argument("--init", default=None)
    args = p.parse_args()
    torch.manual_seed(0)
    rng = np.random.default_rng(0)
    device = "cuda" if torch.cuda.is_available() else "cpu"
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    bc.Batch.conditions = {}
    for path in args.condition.split(","):
        for r in csv.DictReader(open(path)):
            bc.Batch.conditions[(int(r["episode"]), int(r["seat"]))] = (
                float(r["rating"]) if r["rating"] else None, float(r["day_index"]) if r["day_index"] else None)
    d = bc.load(args.arrays)
    bc.Batch.condition_rows = bc.condition_table(d, bc.Batch.conditions)
    margins = results()
    if args.margins:
        for r in csv.DictReader(open(args.margins)):
            margins[(int(r["episode"]), int(r["seat"]))] = float(r["margin"])
    meta = d["meta"]
    label = np.array([margins.get((int(e), int(s)), np.nan) for e, s in meta[:, :2]], dtype=np.float32)
    known = ~np.isnan(label)
    split, day = meta[:, 5], meta[:, 2]
    train = np.where(known & (split == 0))[0]
    valid = np.where(known & (split == 1))[0]
    valid = np.sort(np.random.default_rng(12345).choice(valid, min(len(valid), 60000), replace=False))
    print(f"dawns {len(meta)} labelled {known.sum()} train {len(train)} valid {len(valid)}", flush=True)
    gd = bc.GpuData(d, device) if args.gpu_data else None
    batch = (lambda rows, train=False: bc.gpu_batch(gd, rows, train)) if gd else \
        (lambda rows, train=False: bc.Batch(d, rows, device, train=train))
    mc = label.copy()  # Monte Carlo labels: the reported R^2 is always against the final margin
    if args.td:
        old = ValueNet(args.width).to(device)
        old.load_state_dict(torch.load(Path(args.td) / "value.pt", map_location=device))
        old.eval()
        rows = np.where(known)[0]
        v = np.zeros(len(meta), dtype=np.float32)
        with torch.no_grad():
            for i in range(0, len(rows), 8192):
                v[rows[i:i + 8192]] = old(batch(rows[i:i + 8192]))[0].cpu().numpy() * SCALE
        order = rows[np.lexsort((day[rows], meta[rows, 1], meta[rows, 0]))]
        for i in range(len(order) - 2, -1, -1):  # backwards: the next dawn of the same perspective
            a, b = order[i], order[i + 1]
            if meta[a, 0] == meta[b, 0] and meta[a, 1] == meta[b, 1]:
                label[a] = (1 - args.lam) * v[b] + args.lam * label[b]
        print(f"TD targets: std {label[known].std():.0f} vs Monte Carlo {mc[known].std():.0f}", flush=True)
    target = torch.as_tensor(label, device=device)
    target_win = torch.as_tensor((mc > 0).astype(np.float32), device=device)  # the win head keeps the real outcome
    net = ValueNet(args.width).to(device)
    if args.init:
        net.load_state_dict(torch.load(Path(args.init) / "value.pt", map_location=device))
    opt = torch.optim.AdamW(net.parameters(), lr=args.lr, weight_decay=1e-4)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, args.steps)
    t0 = time.time()

    def predict(rows):
        net.eval()
        m, w = [], []
        with torch.no_grad():
            for i in range(0, len(rows), 4096):
                pm, pw = net(batch(rows[i:i + 4096]))
                m.append(pm.cpu().numpy() * SCALE)
                w.append(torch.sigmoid(pw).cpu().numpy())
        net.train()
        return np.concatenate(m), np.concatenate(w)

    for step in range(1, args.steps + 1):
        rows = np.sort(rng.choice(train, args.batch, replace=False))
        pm, pw = net(batch(rows, train=True))
        y = target[torch.as_tensor(rows, device=device)]
        win = target_win[torch.as_tensor(rows, device=device)]
        loss = F.huber_loss(pm, y / SCALE, delta=0.5) + 0.5 * F.binary_cross_entropy_with_logits(pw, win)
        opt.zero_grad()
        loss.backward()
        opt.step()
        sched.step()
        if step % 2000 == 0 or step == args.steps:
            pm_v, _ = predict(valid)
            y_v = mc[valid]
            r2 = 1 - np.mean((pm_v - y_v) ** 2) / np.var(y_v)
            print(f"step {step} loss {loss.item():.4f} valid R2 {r2:.3f} {time.time() - t0:.0f}s", flush=True)
    torch.save(net.state_dict(), out / "value.pt")
    # Per-day report vs a ridge baseline on the dawn features.
    pm_v, pw_v = predict(valid)
    y_v = mc[valid]
    ridge_rows = np.sort(rng.choice(train, min(len(train), 200000), replace=False))
    x_tr, y_tr = d["global"][ridge_rows].astype(np.float64), mc[ridge_rows].astype(np.float64)
    x_tr = np.hstack([x_tr, np.ones((len(x_tr), 1))])
    coef = np.linalg.solve(x_tr.T @ x_tr + 10.0 * np.eye(x_tr.shape[1]), x_tr.T @ y_tr)
    lin = np.hstack([d["global"][valid].astype(np.float64), np.ones((len(valid), 1))]) @ coef
    report = []
    for dd in range(30):
        m = day[valid] == dd
        if m.sum() < 100:
            continue
        yy = y_v[m]
        r2 = 1 - np.mean((pm_v[m] - yy) ** 2) / np.var(yy)
        r2_lin = 1 - np.mean((lin[m] - yy) ** 2) / np.var(yy)
        acc = np.mean((pw_v[m] > 0.5) == (yy > 0))
        report.append({"day": dd, "n": int(m.sum()), "r2": float(r2), "r2_ridge": float(r2_lin), "win_acc": float(acc)})
        print(f"day {dd:2d} n {m.sum():5d} R2 {r2:.3f} (ridge {r2_lin:.3f}) win acc {acc:.3f}")
    (out / "report.json").write_text(json.dumps({"args": vars(args), "days": report}, indent=1))


if __name__ == "__main__":
    main()
