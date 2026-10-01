"""Opponent-sales forecast network (tools_dc11/forecast_extract data): predicts the opponent's net flow per hour and
product today (24 x 9) from what the agent observes at dawn (scalars, trailing 3-day flow, yesterday's flow and dc11's
own forecast). Loss: squared error weighted by product base price (errors on cheap wheat and fertilizer count less).
Reports validation error of the network, dc11's forecast and the trailing mean, overall and per product.
usage: scripts/train_forecast.py <data dir> <out dir> [--steps N] [--width W]"""
import argparse, glob, json, time
from pathlib import Path
import numpy as np, torch, torch.nn as nn

SCALARS, GRID = 72, 24 * 9
WIDTH = SCALARS + 4 * GRID
BASE = np.array([25, 35, 60, 120, 250, 50, 160, 200, 100], dtype=np.float32)  # engine base prices
NAMES = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]


def load(directory):
    xs = [np.fromfile(f, dtype=np.float32).reshape(-1, WIDTH) for f in sorted(glob.glob(f"{directory}/shard_*.x.f32"))]
    ms = [np.fromfile(f, dtype=np.int64).reshape(-1, 4) for f in sorted(glob.glob(f"{directory}/shard_*.meta.i64"))]
    return np.concatenate(xs), np.concatenate(ms)


class Net(nn.Module):
    def __init__(self, width):
        super().__init__()
        self.body = nn.Sequential(nn.Linear(SCALARS + 3 * GRID, width), nn.ReLU(), nn.Linear(width, width), nn.ReLU(),
                                  nn.Linear(width, GRID))

    def forward(self, x):  # the dc11 forecast plus a learned correction
        return x[:, SCALARS + 2 * GRID:SCALARS + 3 * GRID] + self.body(x)


def weighted_error(pred, target, weight):  # mean over dawns of the price-weighted squared error
    return (((pred - target) ** 2).reshape(-1, 24, 9) * weight).sum((1, 2)).mean()


def per_product(pred, target):
    e = ((pred - target) ** 2).reshape(-1, 24, 9).sum(1).mean(0)
    return {n: float(v) for n, v in zip(NAMES, e)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("data"); ap.add_argument("out")
    ap.add_argument("--steps", type=int, default=20000); ap.add_argument("--width", type=int, default=512)
    ap.add_argument("--batch", type=int, default=1024); ap.add_argument("--lr", type=float, default=1e-3)
    args = ap.parse_args()
    x, meta = load(args.data)
    print(f"{len(x)} dawns", flush=True)
    dev = "cuda" if torch.cuda.is_available() else "cpu"
    inputs = torch.tensor(x[:, :SCALARS + 3 * GRID], device=dev)
    target = torch.tensor(x[:, SCALARS + 3 * GRID:], device=dev)
    split = meta[:, 3]
    train = torch.tensor(np.where(split == 0)[0], device=dev)
    valid = torch.tensor(np.where(split == 1)[0], device=dev)
    weight = torch.tensor(BASE / BASE.mean(), device=dev)[None, None, :]
    net = Net(args.width).to(dev)
    opt = torch.optim.AdamW(net.parameters(), lr=args.lr, weight_decay=1e-4)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, args.steps)
    base_v = inputs[valid, SCALARS + 2 * GRID:]
    trail_v = inputs[valid, SCALARS:SCALARS + GRID]
    ref = {"dc11": float(weighted_error(base_v, target[valid], weight)), "trailing": float(weighted_error(trail_v, target[valid], weight)),
           "zero": float(weighted_error(torch.zeros_like(base_v), target[valid], weight))}
    print("validation weighted error:", {k: round(v, 3) for k, v in ref.items()}, flush=True)
    out = Path(args.out); out.mkdir(parents=True, exist_ok=True)
    g = torch.Generator(device=dev); g.manual_seed(0)
    best, history, t0 = float("inf"), [], time.time()
    for step in range(1, args.steps + 1):
        rows = train[torch.randint(len(train), (args.batch,), device=dev, generator=g)]
        loss = weighted_error(net(inputs[rows]), target[rows], weight)
        opt.zero_grad(); loss.backward(); nn.utils.clip_grad_norm_(net.parameters(), 1.0); opt.step(); sched.step()
        if step % 1000 == 0 or step == args.steps:
            with torch.no_grad():
                pv = torch.cat([net(inputs[valid[i:i + 8192]]) for i in range(0, len(valid), 8192)])
                v = float(weighted_error(pv, target[valid], weight))
            history.append({"step": step, "train": float(loss), "valid": v})
            print(f"step {step} train {float(loss):.3f} valid {v:.3f} (dc11 {ref['dc11']:.3f}) {time.time() - t0:.0f}s", flush=True)
            if v < best:
                best = v
                torch.save(net.state_dict(), out / "forecast.pt")
                prod = {"net": per_product(pv, target[valid]), "dc11": per_product(base_v, target[valid])}
    (out / "history.json").write_text(json.dumps({"args": vars(args), "reference": ref, "best": best, "per_product": prod,
                                                  "history": history}, indent=1))
    print("per product (squared error per dawn), net vs dc11:")
    for n in NAMES:
        print(f"  {n:10} {prod['net'][n]:8.2f} {prod['dc11'][n]:8.2f}")


if __name__ == "__main__":
    main()
