"""Transformer / sequence forecasters with cached inputs, for sweeps.
  prepare: fc_tf.py prepare <dir>   -> <dir>/seq_cache.npz (per (game, opponent) sequence of days 1-28; per day and product
           the 34 dawn features (log-scaled counts) and the previous day's 24 hourly sales; targets: 24 hourly sales per product)
  train:   fc_tf.py train <dir> <name> [options]  -> <dir>/pred_<name>.csv (test rows), <dir>/<name>.pt, metrics line
Models (--model): tf (causal transformer, one sequence per product), joint (causal transformer, one token per day with all
7 products, 7 x 24 outputs), tcn (dilated causal convolutions over days, per product).
Options: --width --layers --heads --dropout --lr --bs --epochs --wd; augmentations: --shift (rotate the opponent's hours by a
multiple of 4 in inputs and targets of a sequence, probability), --crop (train on random sub-sequences starting at day 1,
probability), --noise (Gaussian input noise sd); --weight-extra (loss weight of local/synthetic rows); --seed."""
import argparse, glob, json, sys, time
import numpy as np, pandas as pd, torch
torch.set_num_threads(4)  # shared machine: the CPU side of GPU training needs few threads
from fc_features import A, F, prepare

COUNTS = ["s1", "s2", "s3", "y0", "y1", "y2", "y3", "cum", "stock", "visible", "avail", "txa", "herd", "herd_ready", "own_stock", "f_total"]
HOUR_FEATS = {"band": ["y0", "y1", "y2", "y3"], "fc": ["f0", "f1", "f2", "f3"], "cs": ["cs0", "cs1", "cs2", "cs3"]}


def do_prepare(D):
    d = pd.concat([pd.read_csv(f) for f in sorted(glob.glob(f"{D}/rows_*.csv"))], ignore_index=True)
    t = pd.read_csv(f"{D}/targets.csv", dtype={"episode": str}); t["trace"] = t.trace.str.rsplit("/", n=1).str[1]
    d = d.merge(t[["trace", "target", "split", "source"]], on=["trace", "target"])
    d = prepare(d)
    key = [d.trace, d.target, d["product"]]
    lag1 = d.groupby(key)[A].shift(1).fillna(0).values
    X = d[F].astype(float).copy()
    for c in COUNTS: X[c] = np.sign(X[c]) * np.log1p(np.abs(X[c]))
    for c in ["money", "opp_money"]: X[c] = np.log1p(X[c].clip(lower=0))
    Xf = X.values.astype(np.float32); L1 = np.log1p(lag1.clip(0)).astype(np.float32); Y = d[A].clip(lower=0).values.astype(np.float32)
    # dense tensor [sequence (trace, target), day 1..28, product 1..7]
    seqs = d[["trace", "target"]].drop_duplicates().reset_index(drop=True)
    sid = d.merge(seqs.reset_index().rename(columns={"index": "sid"}), on=["trace", "target"]).sid.values
    n = len(seqs); nf = Xf.shape[1]
    XS = np.zeros((n, 28, 7, nf), np.float32); LS = np.zeros((n, 28, 7, 24), np.float32); YS = np.zeros((n, 28, 7, 24), np.float32)
    M = np.zeros((n, 28, 7), np.float32)
    di = d.day.values - 1; pi = d["product"].values - 1
    XS[sid, di, pi] = Xf; LS[sid, di, pi] = L1; YS[sid, di, pi] = Y; M[sid, di, pi] = 1
    first = d.groupby(["trace", "target"], sort=False).first().reindex(pd.MultiIndex.from_frame(seqs))
    extra = {}
    if "st0" in d:  # the target's inferred stock at the start of each hour (fcst_join.py), log1p
        SS = np.zeros((n, 28, 7, 24), np.float32)
        SS[sid, di, pi] = np.log1p(d[[f"st{h}" for h in range(24)]].clip(lower=0).values.astype(np.float32))
        VS = np.zeros((n, 28, 7, 24), np.float32)
        VS[sid, di, pi] = np.log1p(d[[f"vs{h}" for h in range(24)]].clip(lower=0).values.astype(np.float32))
        extra["S"], extra["V"] = SS, VS
    if "ash0" in d:  # units the target's workers carry beside the shed / elsewhere at the start of each hour (probe/fc_carry.hpp), log1p
        for key, col in (("CA", "ash"), ("CE", "cel")) + ((("CN", "cnr"), ("CF", "cfr")) if "cnr0" in d else ()):  # + near / far (v3 cargo)
            C = np.zeros((n, 28, 7, 24), np.float32)
            C[sid, di, pi] = np.log1p(d[[f"{col}{h}" for h in range(24)]].clip(lower=0).values.astype(np.float32))
            extra[key] = C
    np.savez_compressed(f"{D}/seq_cache.npz", **extra, X=XS, L=LS, Y=YS, M=M, split=first.split.values.astype(str), source=first.source.fillna("").values.astype(str),
                        trace=seqs.trace.values.astype(str), target=seqs.target.values, F=np.array(F))
    print("cached", n, "sequences", XS.shape, flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd"); ap.add_argument("dir"); ap.add_argument("name", nargs="?")
    ap.add_argument("--model", default="tf"); ap.add_argument("--width", type=int, default=192); ap.add_argument("--layers", type=int, default=3)
    ap.add_argument("--heads", type=int, default=4); ap.add_argument("--dropout", type=float, default=0.1); ap.add_argument("--lr", type=float, default=1e-3)
    ap.add_argument("--bs", type=int, default=32)  # games per batch (x7 product sequences)
    ap.add_argument("--epochs", type=int, default=30)
    ap.add_argument("--wd", type=float, default=1e-4)
    ap.add_argument("--shift", type=float, default=0.0); ap.add_argument("--crop", type=float, default=0.0); ap.add_argument("--noise", type=float, default=0.0)
    ap.add_argument("--weight-extra", type=float, default=1.0); ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--weight-lineage", type=float, default=None)  # loss weight of our-lineage opponent games (default: weight-extra)
    ap.add_argument("--val", default="test_top")
    ap.add_argument("--sched", default="cosine")  # cosine | onecycle
    ap.add_argument("--lagnorm", type=int, default=1)  # standardize the previous-day hourly inputs
    a = ap.parse_args()
    if a.cmd == "prepare": return do_prepare(a.dir)
    torch.manual_seed(a.seed); np.random.seed(a.seed)
    dev = "cuda"
    z = np.load(f"{a.dir}/seq_cache.npz", allow_pickle=True)  # own cache file
    X, L, Y, M = z["X"], z["L"], z["Y"], z["M"]
    split, source = z["split"].astype(str), z["source"].astype(str)
    feats = list(z["F"])
    nf = X.shape[-1]
    tr = split == "train"
    mu = X[tr].reshape(-1, nf)[M[tr].reshape(-1) > 0].mean(0); sd = X[tr].reshape(-1, nf)[M[tr].reshape(-1) > 0].std(0) + 1e-6
    Xn = (X - mu) / sd
    lag_mu, lag_sd = np.zeros(24, np.float32), np.ones(24, np.float32)
    if a.lagnorm:
        lm = L[tr].reshape(-1, 24)[M[tr].reshape(-1) > 0]
        lag_mu, lag_sd = lm.mean(0), lm.std(0) + 1e-6
        L = (L - lag_mu) / lag_sd
    W = np.where(np.char.startswith(source, "local") | np.char.startswith(source, "synth"), a.weight_extra, 1.0).astype(np.float32)
    W = np.where(np.char.startswith(source, "lineage"), a.weight_lineage if a.weight_lineage is not None else a.weight_extra, W).astype(np.float32)
    # indices of hour-structured scalar features (rotated with the hours under --shift)
    band_idx = [feats.index(f) for f in HOUR_FEATS["band"]]; fc_idx = [feats.index(f) for f in HOUR_FEATS["fc"]]; cs_idx = [feats.index(f) for f in HOUR_FEATS["cs"]]
    T = lambda v: torch.tensor(v, device=dev)
    Xt, Lt, Yt, Mt, Wt = T(Xn[tr]), T(L[tr]), T(Y[tr]), T(M[tr]), T(W[tr])
    va = split == a.val
    Xv, Lv, Yv, Mv = T(Xn[va]), T(L[va]), T(Y[va]), T(M[va])
    joint = a.model == "joint"
    n_in = (nf + 24 + 7) * (7 if joint else 1)
    n_out = 24 * (7 if joint else 1)

    class TF(torch.nn.Module):
        def __init__(s):
            super().__init__()
            s.inp = torch.nn.Linear(n_in, a.width); s.pos = torch.nn.Parameter(torch.zeros(28, a.width))
            layer = torch.nn.TransformerEncoderLayer(a.width, a.heads, a.width * 4, dropout=a.dropout, batch_first=True, norm_first=True)
            s.enc = torch.nn.TransformerEncoder(layer, a.layers); s.out = torch.nn.Sequential(torch.nn.LayerNorm(a.width), torch.nn.Linear(a.width, n_out))
        def forward(s, x):
            Lq = x.shape[1]; mask = torch.triu(torch.full((Lq, Lq), float("-inf"), device=x.device), 1)
            return s.out(s.enc(s.inp(x) + s.pos[:Lq], mask=mask))

    class TCN(torch.nn.Module):
        def __init__(s):
            super().__init__()
            s.inp = torch.nn.Linear(n_in, a.width)
            s.convs = torch.nn.ModuleList([torch.nn.Conv1d(a.width, a.width, 2, dilation=2 ** i) for i in range(a.layers)])
            s.out = torch.nn.Sequential(torch.nn.LayerNorm(a.width), torch.nn.Linear(a.width, n_out))
        def forward(s, x):
            h = s.inp(x).transpose(1, 2)
            for i, c in enumerate(s.convs):
                h = h + torch.nn.functional.silu(c(torch.nn.functional.pad(h, (2 ** i, 0))))
            return s.out(h.transpose(1, 2))

    net = (TCN() if a.model == "tcn" else TF()).to(dev)
    prod_eye = torch.eye(7, device=dev)

    def inputs(x, l):  # x [B,28,7,nf], l [B,28,7,24] -> model input and per-product layout
        B = x.shape[0]
        pe = prod_eye.expand(B, 28, 7, 7)
        z = torch.cat([x, l, pe], -1)  # [B,28,7,nf+31]
        return z.reshape(B, 28, -1) if joint else z.permute(0, 2, 1, 3).reshape(B * 7, 28, -1)

    def outputs(o, B):  # -> [B,28,7,24] log-rates
        return o.reshape(B, 28, 7, 24) if joint else o.reshape(B, 7, 28, 24).permute(0, 2, 1, 3)

    def augment(x, l, y, m):
        if a.shift > 0:  # rotate the opponent's hours by k*4 in lags, targets and the hour-structured features
            B = x.shape[0]; sel = torch.rand(B, device=dev) < a.shift
            if sel.any():
                k = int(np.random.choice([4, 8, 12, 16, 20]))
                l = l.clone(); y = y.clone(); x = x.clone()
                l[sel] = torch.roll(l[sel], k, -1); y[sel] = torch.roll(y[sel], k, -1)
                kb = int(round(k / 6)) % 4  # the 6-hour band features move by k/6 bands (rounded)
                if kb:
                    part = x[sel]  # boolean indexing copies: edit, then write back
                    for idx in (band_idx, fc_idx, cs_idx):
                        part[..., idx] = torch.roll(part[..., idx], kb, -1)
                    x[sel] = part
        if a.noise > 0: x = x + a.noise * torch.randn_like(x)
        if a.crop > 0 and np.random.rand() < a.crop:  # train on days 1..c
            c = np.random.randint(8, 29); m = m.clone(); m[:, c:] = 0
        return x, l, y, m

    def loss_of(x, l, y, m, w=None):
        B = x.shape[0]; lr = outputs(net(inputs(x, l)), B)
        nll = (torch.exp(lr) - y * lr).sum(-1)  # [B,28,7]
        ww = m if w is None else m * w[:, None, None]
        return (nll * ww).sum() / ww.sum()

    opt = torch.optim.AdamW(net.parameters(), lr=a.lr, weight_decay=a.wd)
    steps = a.epochs * ((len(Xt) + a.bs - 1) // a.bs)
    sched = (torch.optim.lr_scheduler.OneCycleLR(opt, max_lr=a.lr, total_steps=steps) if a.sched == "onecycle"
             else torch.optim.lr_scheduler.CosineAnnealingLR(opt, steps))
    best, best_state, t0 = 1e9, None, time.time()
    for ep in range(a.epochs):
        net.train(); perm = torch.randperm(len(Xt), device=dev)
        for i in range(0, len(perm), a.bs):
            b = perm[i:i + a.bs]
            x, l, y, m = augment(Xt[b], Lt[b], Yt[b], Mt[b])
            opt.zero_grad(); loss = loss_of(x, l, y, m, Wt[b]); loss.backward()
            torch.nn.utils.clip_grad_norm_(net.parameters(), 1.0); opt.step(); sched.step()
        net.eval()
        with torch.no_grad():
            vl = float(sum(loss_of(Xv[i:i + 512], Lv[i:i + 512], Yv[i:i + 512], Mv[i:i + 512]) * Mv[i:i + 512].sum() for i in range(0, len(Xv), 512)) / Mv.sum())
        if vl < best: best, best_state = vl, {k: v.clone() for k, v in net.state_dict().items()}
    net.load_state_dict(best_state); net.eval()
    te = ~tr
    with torch.no_grad():
        Xe, Le = T(Xn[te]), T(L[te])
        rate = torch.cat([torch.exp(outputs(net(inputs(Xe[i:i + 512], Le[i:i + 512])), len(Xe[i:i + 512]))) for i in range(0, len(Xe), 512)]).cpu().numpy()
    # metrics per split (Poisson NLL per product-day, band error)
    Yte, Mte, spl = Y[te], M[te], split[te]
    res = {}
    for s in np.unique(spl):
        k = spl == s; m = Mte[k] > 0
        lam = np.clip(rate[k][m], 1e-6, None); yy = Yte[k][m]
        res[s] = dict(poisson=round(float((lam - yy * np.log(lam)).sum(-1).mean()), 4),
                      band=round(float(np.abs(lam.reshape(-1, 6, 4).sum(2) - yy.reshape(-1, 6, 4).sum(2)).sum(1).mean()), 3),
                      daily_mae=round(float(np.abs(lam.sum(-1) - yy.sum(-1)).mean()), 3))
    n_params = sum(p.numel() for p in net.parameters())
    print(json.dumps({"name": a.name, "args": {k: v for k, v in vars(a).items() if k not in ("cmd", "dir", "name")}, "params": n_params,
                      "best_val": round(best, 4), "secs": round(time.time() - t0), "metrics": res}), flush=True)
    # prediction file in the fc_model.py row format
    tr_ids, tg = z["trace"][te], z["target"][te]
    rows = []
    for j in range(len(tr_ids)):
        for di in range(28):
            for pi in range(7):
                if Mte[j, di, pi] > 0:
                    rows.append([tr_ids[j], int(tg[j]), di + 1, pi + 1, spl[j], float(Yte[j, di, pi].sum())] + list(rate[j, di, pi]) + list(Yte[j, di, pi]))
    cols = ["trace", "target", "day", "product", "split", "total"] + [f"p{h}" for h in range(24)] + A
    r = pd.DataFrame(rows, columns=cols); r["pred_total"] = r[[f"p{h}" for h in range(24)]].sum(axis=1)
    for c in ["f_total", "f0", "f1", "f2", "f3", "cs0", "cs1", "cs2", "cs3", "cdays"]: r[c] = 0.0
    r.to_csv(f"{a.dir}/pred_{a.name}.csv", index=False)
    torch.save({"state": best_state, "mu": mu, "sd": sd, "lag_mu": lag_mu, "lag_sd": lag_sd, "args": vars(a), "features": feats,
                "counts": COUNTS}, f"{a.dir}/{a.name}.pt")


if __name__ == "__main__":
    main()
