"""Transformer / sequence forecasters with cached inputs, for sweeps.
  prepare: fc_tf.py prepare <dir>   -> <dir>/seq_cache.npz (per (game, opponent) sequence of days 1-28; per day and product
           the 34 dawn features (log-scaled counts) and the previous day's 24 hourly sales; targets: 24 hourly sales per product)
  train:   fc_tf.py train <dir> <name> [options]  -> <dir>/pred_<name>.csv (test rows), <dir>/<name>.pt, metrics line
Models (--model): tf (causal transformer, one sequence per product), joint (causal transformer, one token per day with all
7 products, 7 x 24 outputs), tcn (dilated causal convolutions over days, per product).
Options: --width --layers --heads --dropout --lr --bs --epochs --wd; augmentations: --shift (rotate the opponent's hours by a
multiple of 4 in inputs and targets of a sequence, probability), --crop (train on random sub-sequences starting at day 1,
probability), --noise (Gaussian input noise sd); --weight-extra (loss weight of local/synthetic rows); --seed.
Intra-day head (this copy): a second output conditioned on today's opponent sales before a cut hour c (random per row in
training) predicts the hours >= c; the metrics compare it with the dawn head on those hours for cuts 0, 4, ..., 20."""
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
    np.savez_compressed(f"{D}/seq_cache.npz", X=XS, L=LS, Y=YS, M=M, split=first.split.values.astype(str), source=first.source.fillna("").values.astype(str),
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
    ap.add_argument("--intra-width", type=int, default=0)  # hidden width of the intra-day head (0: the model width)
    ap.add_argument("--exclude", default="")  # training sequences whose source starts with this are not used (control runs)
    ap.add_argument("--stockin", type=int, default=0)  # 1: the intra-day head also gets the target's inferred stock and visible output at the cut hour
    ap.add_argument("--exclude-teams", default="")  # csv (column team): training sequences with these target teams are dropped
    ap.add_argument("--weight-teams", default="")  # "csv:w": sequences whose target team is listed (column team) get loss weight w
    ap.add_argument("--weight-old", type=float, default=1.0)  # loss weight of sources starting with "old_" (Sep 8-17 Kaggle games)
    ap.add_argument("--xseen", type=int, default=0)  # 1: the intra-day head also gets today's sales before the cut summed over the 7 products
    ap.add_argument("--cumloss", type=float, default=0.0)  # + this x sum over hours of |cumulative expected - cumulative actual| (both heads)
    ap.add_argument("--dw", type=int, default=0)  # 1: row loss weight 0.5 + log1p(observer's dawn stock of the product) in training
    ap.add_argument("--pw", default="")  # product loss weights "p:w,..." (p = 1 carrot .. 7 wool), e.g. "6:3,7:3"
    ap.add_argument("--carryin", type=int, default=0)  # 1: the intra-day head also gets the target's carried units beside the shed / elsewhere at the cut hour; 2: + near / far (1-2 / 3+ tiles, v3 drop rule)
    ap.add_argument("--next-weight", type=float, default=1.0)  # loss weight of the next-morning outputs
    ap.add_argument("--next", type=int, default=0)  # 12: the intra-day head also predicts tomorrow's first 12 hours (hold value of stock kept overnight)
    ap.add_argument("--xlag", type=int, default=0)  # 1: also the previous day's hourly sales summed over the 7 products (log1p)
    a = ap.parse_args()
    if a.cmd == "prepare": return do_prepare(a.dir)
    torch.manual_seed(a.seed); np.random.seed(a.seed)
    # the GPU is shared with BC training: data stay in CPU memory, batches move to the GPU, and this process is capped
    dev = "cuda"
    if torch.cuda.mem_get_info()[0] < 2e9: sys.exit("GPU: less than 2 GB free")
    torch.cuda.set_per_process_memory_fraction(1.2e9 / torch.cuda.mem_get_info()[1])
    z = np.load(f"{a.dir}/seq_cache.npz", allow_pickle=True)  # own cache file
    X, L, Y, M = z["X"], z["L"], z["Y"], z["M"]
    chans = ([z["S"], z["V"]] if a.stockin else []) + ([z["CA"], z["CE"]] if a.carryin else []) + ([z["CN"], z["CF"]] if a.carryin >= 2 else [])  # log1p stock, visible, carried per hour (2: + near / far)
    S = np.stack(chans, -1) if chans else np.zeros((len(X), 1, 1, 1, 1), np.float32)
    split, source = z["split"].astype(str), z["source"].astype(str)
    feats = list(z["F"])
    nf = X.shape[-1]
    tr = split == "train"
    if a.exclude: tr &= ~np.char.startswith(source, a.exclude)
    if a.exclude_teams:
        teams = set(pd.read_csv(a.exclude_teams).team)
        t = pd.read_csv(f"{a.dir}/targets.csv", dtype={"episode": str}); t["base"] = t.trace.str.rsplit("/", n=1).str[1]
        team_of = {(b, int(g)): tm for b, g, tm in zip(t.base, t.target, t.team)}
        drop = np.array([team_of.get((tr_, int(g)), "") in teams for tr_, g in zip(z["trace"], z["target"])])
        print(f"exclude-teams: {(tr & drop).sum()} training sequences dropped", file=sys.stderr, flush=True)
        tr &= ~drop
    mu = X[tr].reshape(-1, nf)[M[tr].reshape(-1) > 0].mean(0); sd = X[tr].reshape(-1, nf)[M[tr].reshape(-1) > 0].std(0) + 1e-6
    Xn = (X - mu) / sd
    lag_mu, lag_sd = np.zeros(24, np.float32), np.ones(24, np.float32)
    if a.lagnorm:
        lm = L[tr].reshape(-1, 24)[M[tr].reshape(-1) > 0]
        lag_mu, lag_sd = lm.mean(0), lm.std(0) + 1e-6
        L = (L - lag_mu) / lag_sd
    W = np.where(np.char.startswith(source, "local") | np.char.startswith(source, "synth"), a.weight_extra, 1.0).astype(np.float32)
    W = np.where(np.char.startswith(source, "lineage"), a.weight_lineage if a.weight_lineage is not None else a.weight_extra, W).astype(np.float32)
    W = np.where(np.char.startswith(source, "old_"), a.weight_old, W).astype(np.float32)
    if a.weight_teams:
        path, w = a.weight_teams.rsplit(":", 1)
        teams = set(pd.read_csv(path).team)
        t = pd.read_csv(f"{a.dir}/targets.csv", dtype={"episode": str}); t["base"] = t.trace.str.rsplit("/", n=1).str[1]
        team_of = {(b, int(g)): tm for b, g, tm in zip(t.base, t.target, t.team)}
        hit = np.array([team_of.get((tr, int(g)), "") in teams for tr, g in zip(z["trace"], z["target"])])
        W = np.where(hit, W * float(w), W).astype(np.float32)
        print(f"weight-teams: {hit.mean():.1%} of sequences x{w}", file=sys.stderr, flush=True)
    # indices of hour-structured scalar features (rotated with the hours under --shift)
    band_idx = [feats.index(f) for f in HOUR_FEATS["band"]]; fc_idx = [feats.index(f) for f in HOUR_FEATS["fc"]]; cs_idx = [feats.index(f) for f in HOUR_FEATS["cs"]]
    T = lambda v: torch.tensor(v)
    G = lambda t: t.to(dev, non_blocking=True)
    lag_mu_t, lag_sd_t = G(T(lag_mu)), G(T(lag_sd))
    Lraw = np.expm1(L * lag_sd + lag_mu).clip(0) if a.lagnorm else np.expm1(L).clip(0)
    xl = np.log1p(Lraw[tr].sum(2)).reshape(-1, 24)  # standardization of the summed lags over training days
    xlag_mu, xlag_sd = G(T(xl.mean(0))), G(T(xl.std(0) + 1e-6))
    Xt, Lt, Yt, Mt, Wt, St = T(Xn[tr]), T(L[tr]), T(Y[tr]), T(M[tr]), T(W[tr]), T(S[tr])
    Dt = T((0.5 + X[tr][..., feats.index("own_stock")].clip(min=0)) if a.dw else np.ones(M[tr].shape, np.float32))  # own_stock is log1p-scaled
    va = split == a.val
    Xv, Lv, Yv, Mv, Sv = T(Xn[va]), T(L[va]), T(Y[va]), T(M[va]), T(S[va])
    joint = a.model == "joint"
    n_in = (nf + 24 + 7 + 24 * a.xlag) * (7 if joint else 1)
    n_out = 24 * (7 if joint else 1)

    class TF(torch.nn.Module):
        def __init__(s):
            super().__init__()
            s.inp = torch.nn.Linear(n_in, a.width); s.pos = torch.nn.Parameter(torch.zeros(28, a.width))
            layer = torch.nn.TransformerEncoderLayer(a.width, a.heads, a.width * 4, dropout=a.dropout, batch_first=True, norm_first=True)
            s.enc = torch.nn.TransformerEncoder(layer, a.layers); s.out = torch.nn.Sequential(torch.nn.LayerNorm(a.width), torch.nn.Linear(a.width, n_out))
            iw = a.intra_width or a.width
            s.intra = torch.nn.Sequential(torch.nn.Linear(a.width + 48 + 2 * a.stockin + 2 * a.carryin + 24 * a.xseen, iw), torch.nn.SiLU(), torch.nn.Linear(iw, iw), torch.nn.SiLU(), torch.nn.Linear(iw, 24 + a.next))
        def forward(s, x):  # hidden states
            Lq = x.shape[1]; mask = torch.triu(torch.full((Lq, Lq), float("-inf"), device=x.device), 1)
            return s.enc(s.inp(x) + s.pos[:Lq], mask=mask)

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

    assert a.model == "tf"
    net = TF().to(dev)
    prod_eye = torch.eye(7, device=dev)

    def inputs(x, l):  # x [B,28,7,nf], l [B,28,7,24] -> model input and per-product layout
        B = x.shape[0]
        pe = prod_eye.expand(B, 28, 7, 7)
        z = torch.cat([x, l, pe], -1)  # [B,28,7,nf+31]
        if a.xlag:  # all products' sales per hour yesterday (from the raw lags)
            raw = torch.expm1(l * lag_sd_t + lag_mu_t).clamp(min=0) if a.lagnorm else torch.expm1(l).clamp(min=0)
            z = torch.cat([z, ((torch.log1p(raw.sum(2, keepdim=True)) - xlag_mu) / xlag_sd).expand(B, 28, 7, 24)], -1)
        return z.reshape(B, 28, -1) if joint else z.permute(0, 2, 1, 3).reshape(B * 7, 28, -1)

    def outputs(o, B):  # -> [B,28,7,24] log-rates
        return o.reshape(B, 28, 7, 24) if joint else o.reshape(B, 7, 28, 24).permute(0, 2, 1, 3)

    def augment(x, l, y, m, st):
        if a.shift > 0:  # rotate the opponent's hours by k*4 in lags, targets, stock and the hour-structured features
            B = x.shape[0]; sel = torch.rand(B, device=dev) < a.shift
            if sel.any():
                k = int(np.random.choice([4, 8, 12, 16, 20]))
                l = l.clone(); y = y.clone(); x = x.clone(); st = st.clone()
                l[sel] = torch.roll(l[sel], k, -1); y[sel] = torch.roll(y[sel], k, -1)
                if a.stockin or a.carryin: st[sel] = torch.roll(st[sel], k, -2)
                kb = int(round(k / 6)) % 4  # the 6-hour band features move by k/6 bands (rounded)
                if kb:
                    part = x[sel]  # boolean indexing copies: edit, then write back
                    for idx in (band_idx, fc_idx, cs_idx):
                        part[..., idx] = torch.roll(part[..., idx], kb, -1)
                    x[sel] = part
        if a.noise > 0: x = x + a.noise * torch.randn_like(x)
        if a.crop > 0 and np.random.rand() < a.crop:  # train on days 1..c
            c = np.random.randint(8, 29); m = m.clone(); m[:, c:] = 0
        return x, l, y, m, st

    hours = torch.arange(24, device=dev)
    pw = torch.ones(7, device=dev)
    for item in filter(None, a.pw.split(",")):
        p, v = item.split(":"); pw[int(p) - 1] = float(v)

    def intra(hid, y, cut, st):  # hid [B,28,7,W], y [B,28,7,24], cut [B,28,7], st log1p stock, visible [B,28,7,24,2] -> log-rates given hours < cut
        seen = (hours < cut[..., None]).float()
        z = [hid, torch.log1p(y) * seen, seen]
        nc = 2 * a.stockin + 2 * a.carryin  # stock, visible, carried beside the shed / elsewhere at the cut hour
        if nc: z.append(torch.gather(st, -2, cut[..., None, None].long().expand(*cut.shape, 1, nc))[..., 0, :])
        if a.xseen: z.append(torch.log1p(y.sum(2, keepdim=True).expand_as(y)) * seen)  # all products, hours < cut
        return net.intra(torch.cat(z, -1))

    def loss_of(x, l, y, m, st, w=None, dw=None):
        B = x.shape[0]; hid = net(inputs(x, l))
        hid = hid.reshape(B, 7, 28, -1).permute(0, 2, 1, 3)  # [B,28,7,W]
        lr = net.out(hid)
        nll = (torch.exp(lr) - y * lr).sum(-1)  # [B,28,7]
        ww = (m if w is None else m * w[:, None, None]) * (pw if w is not None else 1)  # product weights in training only
        if dw is not None: ww = ww * dw
        cut = torch.randint(0, 24, m.shape, device=dev)
        li = intra(hid, y, cut, st)
        ln, li = li[..., 24:], li[..., :24]
        after = (hours >= cut[..., None]).float()
        nll_i = ((torch.exp(li) - y * li) * after).sum(-1)
        if a.cumloss > 0 and w is not None:  # training only: the cumulative profile the seller's price depends on
            nll = nll + a.cumloss * (torch.cumsum(torch.exp(lr) - y, -1)).abs().sum(-1)
            nll_i = nll_i + a.cumloss * (torch.cumsum((torch.exp(li) - y) * after, -1)).abs().sum(-1)
        loss = ((nll + nll_i) * ww).sum() / ww.sum()
        if a.next:  # tomorrow's first hours (outputs 0-23) and, with --next > 24, the following days' (24-47, 48-71, ...) from the state at today's cut
            for k in range((a.next + 23) // 24):
                lo, hi = 24 * k, min(a.next, 24 * k + 24)
                d = k + 1  # days ahead
                yn = torch.cat([y[:, d:, :, :hi - lo], torch.zeros_like(y[:, :d, :, :hi - lo])], 1)
                wn = ww * torch.cat([m[:, d:], torch.zeros_like(m[:, :d])], 1)
                l = ln[..., lo:hi]
                loss = loss + a.next_weight * ((torch.exp(l) - yn * l).sum(-1) * wn).sum() / wn.sum().clamp(min=1)
        return loss

    opt = torch.optim.AdamW(net.parameters(), lr=a.lr, weight_decay=a.wd)
    steps = a.epochs * ((len(Xt) + a.bs - 1) // a.bs)
    sched = (torch.optim.lr_scheduler.OneCycleLR(opt, max_lr=a.lr, total_steps=steps) if a.sched == "onecycle"
             else torch.optim.lr_scheduler.CosineAnnealingLR(opt, steps))
    best, best_state, t0 = 1e9, None, time.time()
    for ep in range(a.epochs):
        net.train(); perm = torch.randperm(len(Xt))
        for i in range(0, len(perm), a.bs):
            b = perm[i:i + a.bs]
            x, l, y, m, st = augment(G(Xt[b]), G(Lt[b]), G(Yt[b]), G(Mt[b]), G(St[b]))
            opt.zero_grad(); loss = loss_of(x, l, y, m, st, G(Wt[b]), G(Dt[b])); loss.backward()
            torch.nn.utils.clip_grad_norm_(net.parameters(), 1.0); opt.step(); sched.step()
        net.eval()
        with torch.no_grad():
            vl = float(sum(loss_of(G(Xv[i:i + 128]), G(Lv[i:i + 128]), G(Yv[i:i + 128]), G(Mv[i:i + 128]), G(Sv[i:i + 128])).cpu() * Mv[i:i + 128].sum() for i in range(0, len(Xv), 128)) / Mv.sum())
        if vl < best: best, best_state = vl, {k: v.clone() for k, v in net.state_dict().items()}
        print(f"epoch {ep} val {vl:.4f} {time.time() - t0:.0f}s", file=sys.stderr, flush=True)
    net.load_state_dict(best_state); net.eval()
    torch.save({"intra": True, "xlag_mu": xlag_mu.cpu().numpy(), "xlag_sd": xlag_sd.cpu().numpy(), "state": best_state, "mu": mu, "sd": sd, "lag_mu": lag_mu, "lag_sd": lag_sd, "args": vars(a), "features": feats,
                "counts": COUNTS}, f"{a.dir}/{a.name}.pt")  # before the metrics: a metrics bug must not lose the model
    te = ~tr
    with torch.no_grad():
        Xe, Le, Ye, Se = T(Xn[te]), T(L[te]), T(Y[te]), T(S[te])
        cuts = list(range(0, 24, 4))
        rate, irate = [], {c: [] for c in cuts}
        for i in range(0, len(Xe), 128):
            n = len(Xe[i:i + 128]); y = G(Ye[i:i + 128])
            hid = net(inputs(G(Xe[i:i + 128]), G(Le[i:i + 128]))).reshape(n, 7, 28, -1).permute(0, 2, 1, 3)
            rate.append(torch.exp(net.out(hid)).cpu())
            for c in cuts: irate[c].append(torch.exp(intra(hid, y, torch.full(hid.shape[:3], c, device=dev), G(Se[i:i + 128]))).cpu())
        rate = torch.cat(rate).numpy(); irate = {c: torch.cat(v).numpy() for c, v in irate.items()}
        nrate = {c: v[..., 24:] for c, v in irate.items()}; irate = {c: v[..., :24] for c, v in irate.items()}
    # metrics per split (Poisson NLL per product-day, band error)
    Yte, Mte, spl = Y[te], M[te], split[te]
    res = {}
    for s in np.unique(spl):
        k = spl == s; m = Mte[k] > 0
        lam = np.clip(rate[k][m], 1e-6, None); yy = Yte[k][m]
        res[s] = dict(poisson=round(float((lam - yy * np.log(lam)).sum(-1).mean()), 4),
                      band=round(float(np.abs(lam.reshape(-1, 6, 4).sum(2) - yy.reshape(-1, 6, 4).sum(2)).sum(1).mean()), 3),
                      daily_mae=round(float(np.abs(lam.sum(-1) - yy.sum(-1)).mean()), 3))
        for c in cuts:  # hours >= c: dawn head vs intra-day head (poisson, then daily remaining-units error)
            li = np.clip(irate[c][k][m][:, c:], 1e-6, None); ld = lam[:, c:]; yr = yy[:, c:]
            res[s][f"c{c}"] = [round(float((q - yr * np.log(q)).sum(-1).mean()), 4) for q in (ld, li)] + \
                              [round(float(np.abs(q.sum(-1) - yr.sum(-1)).mean()), 3) for q in (ld, li)]
        if a.next:  # tomorrow h0-11 at cut c: today's dawn forecast h0-11 (what dc11 wraps) vs the next-morning head vs tomorrow's dawn head
            nh = min(a.next, 12)  # tomorrow's hours 0-11 (what the next-morning hold value uses)
            both = (Mte[k][:, :-1] > 0) & (Mte[k][:, 1:] > 0); yt = Yte[k][:, 1:][both][:, :nh]
            wrap = np.clip(rate[k][:, :-1][both][:, :nh], 1e-6, None); dawn1 = np.clip(rate[k][:, 1:][both][:, :nh], 1e-6, None)
            nll = lambda q: round(float((q - yt * np.log(q)).sum(-1).mean()), 4)
            mae = lambda q: round(float(np.abs(q.sum(-1) - yt.sum(-1)).mean()), 3)
            res[s]["next"] = {"wrap": [nll(wrap), mae(wrap)], "tomorrow_dawn": [nll(dawn1), mae(dawn1)]}
            for c in cuts:
                q = np.clip(nrate[c][k][:, :-1][both][:, :nh], 1e-6, None)
                res[s]["next"][f"c{c}"] = [nll(q), mae(q)]
            if a.next > 24:  # R^2 of daily totals: tomorrow and the day after, from the head at cuts 0 / 12 / 20; tomorrow's own dawn forecast
                r2 = lambda yv, pv: round(float(1 - ((yv - pv) ** 2).sum() / ((yv - yv.mean()) ** 2).sum()), 3)
                b1 = (Mte[k][:, :-1] > 0) & (Mte[k][:, 1:] > 0); b2 = (Mte[k][:, :-2] > 0) & (Mte[k][:, 2:] > 0)
                t1 = Yte[k][:, 1:][b1].sum(-1); t2 = Yte[k][:, 2:][b2].sum(-1)
                res[s]["days"] = {"tomorrow_dawn_r2": r2(t1, rate[k][:, 1:][b1].sum(-1))}
                for c in (0, 12, 20):
                    res[s]["days"][f"c{c}"] = [r2(Yte[k][:, d:][bd].sum(-1), nrate[c][k][:, :-d][bd][:, 24 * d - 24:24 * d].sum(-1))
                                               for d in range(1, a.next // 24 + 1) for bd in [(Mte[k][:, :-d] > 0) & (Mte[k][:, d:] > 0)]]
                if a.next >= 72:  # cumulative supply over days +1..+D (what a market's multi-day recovery depends on), head at cut 0
                    D = a.next // 24; bd = (Mte[k][:, :-D] > 0) & (Mte[k][:, D:] > 0)
                    ycum = sum(Yte[k][:, d:28 - D + d][bd].sum(-1) for d in range(1, D + 1))
                    res[s]["days"][f"cum{D}_c0"] = r2(ycum, nrate[0][k][:, :-D][bd][:, :24 * D].sum(-1))
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


if __name__ == "__main__":
    main()
