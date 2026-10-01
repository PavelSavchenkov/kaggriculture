"""Scores a trained opponent-sales forecaster (work/sep26_wide_losses/fc_tf_intra.py .pt: tf model, dawn head + intra-day head) on new
rows (probe/fc_rows output) with the model's own saved normalisation. Per (trace, target, day, product) and decision hour D in 0 / 6 /
12 / 18 (D = 0: the dawn head; D > 0: the intra-day head given the target's actual sales before D), the predicted and actual units in
the hour bands 0-2, 3-11, 12-20, 21-23 (hours >= D only). Raw Poisson means, not the DP's rounded units. Models with a next-morning
head (--next >= 12): also decision 100 + c for c in 12 / 18 / 20 = tomorrow's h0-2 (band 0) and h3-11 (band 1) forecast at hour c.
usage: fc_eval.py <model.pt> <eval dir with rows_*.csv and targets.csv (trace, target, split, source)> <out.csv>"""
import sys
import numpy as np
import pandas as pd
import torch

sys.path.insert(0, "/home/pavel/Programming/kaggriculture/work/sep26_wide_losses")
import fc_tf as P  # noqa: E402  (do_prepare writes the stock / visible channels the stockin model needs)

BANDS = [(0, 3), (3, 12), (12, 21), (21, 24)]
CUTS = [0, 6, 12, 18]
NEXT_CUTS = [12, 18, 20]  # next-morning head (models with --next): tomorrow's h0-2 / h3-11 from these cuts, decision = 100 + cut


def main():
    pt, D, out = sys.argv[1], sys.argv[2], sys.argv[3]
    ck = torch.load(pt, map_location="cpu", weights_only=False)
    a = ck["args"]
    assert a["model"] == "tf" and not a.get("carryin"), "only the tf model without carry inputs"
    P.do_prepare(D)
    z = np.load(f"{D}/seq_cache.npz", allow_pickle=True)
    assert list(z["F"]) == list(ck["features"]), "feature list differs from the model's"
    X, L, Y, M = z["X"], z["L"], z["Y"], z["M"]
    S = np.stack([z["S"], z["V"]], -1) if a["stockin"] else np.zeros((len(X), 1, 1, 1, 1), np.float32)
    nf = X.shape[-1]
    Xn = (X - ck["mu"]) / ck["sd"]
    Ln = (L - ck["lag_mu"]) / ck["lag_sd"] if a["lagnorm"] else L
    W = a["width"]
    xlag = bool(a.get("xlag"))
    n_in = nf + 24 + 7 + 24 * xlag
    lag_mu, lag_sd = torch.tensor(ck["lag_mu"]), torch.tensor(ck["lag_sd"])
    xlag_mu, xlag_sd = (torch.tensor(ck["xlag_mu"]), torch.tensor(ck["xlag_sd"])) if xlag else (None, None)

    class TF(torch.nn.Module):
        def __init__(s):
            super().__init__()
            s.inp = torch.nn.Linear(n_in, W); s.pos = torch.nn.Parameter(torch.zeros(28, W))
            layer = torch.nn.TransformerEncoderLayer(W, a["heads"], W * 4, dropout=a["dropout"], batch_first=True, norm_first=True)
            s.enc = torch.nn.TransformerEncoder(layer, a["layers"]); s.out = torch.nn.Sequential(torch.nn.LayerNorm(W), torch.nn.Linear(W, 24))
            iw = a["intra_width"] or W
            s.intra = torch.nn.Sequential(torch.nn.Linear(W + 48 + 2 * a["stockin"] + 24 * a["xseen"], iw), torch.nn.SiLU(),
                                          torch.nn.Linear(iw, iw), torch.nn.SiLU(), torch.nn.Linear(iw, 24 + a.get("next", 0)))

        def forward(s, x):
            Lq = x.shape[1]; mask = torch.triu(torch.full((Lq, Lq), float("-inf")), 1)
            return s.enc(s.inp(x) + s.pos[:Lq], mask=mask)

    net = TF(); net.load_state_dict(ck["state"]); net.eval()
    eye = torch.eye(7)
    hours = torch.arange(24)
    rows = []
    with torch.no_grad():
        for i in range(0, len(X), 64):
            x, l, y, st = (torch.tensor(v[i:i + 64]) for v in (Xn, Ln, Y, S))
            B = x.shape[0]
            zin = torch.cat([x, l, eye.expand(B, 28, 7, 7)], -1)
            if xlag:  # yesterday's hourly sales of all 7 products (as fc_tf_intra.py inputs())
                raw = torch.expm1(l * lag_sd + lag_mu).clamp(min=0) if a["lagnorm"] else torch.expm1(l).clamp(min=0)
                zin = torch.cat([zin, ((torch.log1p(raw.sum(2, keepdim=True)) - xlag_mu) / xlag_sd).expand(B, 28, 7, 24)], -1)
            zin = zin.permute(0, 2, 1, 3).reshape(B * 7, 28, -1)
            hid = net(zin).reshape(B, 7, 28, -1).permute(0, 2, 1, 3)
            preds = {0: torch.exp(net.out(hid))}
            nexts = {}
            for c in sorted(set(CUTS[1:]) | (set(NEXT_CUTS) if a.get("next", 0) >= 12 else set())):
                cut = torch.full(hid.shape[:3], c)
                seen = (hours < cut[..., None]).float()
                zz = [hid, torch.log1p(y) * seen, seen]
                if a["stockin"]: zz.append(torch.gather(st, -2, cut[..., None, None].expand(*cut.shape, 1, 2))[..., 0, :])
                if a["xseen"]: zz.append(torch.log1p(y.sum(2, keepdim=True).expand_as(y)) * seen)
                o = net.intra(torch.cat(zz, -1))
                preds[c] = torch.exp(o[..., :24])
                if a.get("next", 0) >= 12: nexts[c] = torch.exp(o[..., 24:36])
            m = M[i:i + 64] > 0
            for j, di, pi in zip(*np.nonzero(m)):
                base = [str(z["trace"][i + j]), int(z["target"][i + j]), str(z["source"][i + j]), di + 1, pi + 1]
                if nexts and di + 1 < 28 and M[i + j, di + 1, pi] > 0:
                    for c in NEXT_CUTS:
                        pr, ac = nexts[c][j, di, pi].numpy(), Y[i + j, di + 1, pi]
                        rows.append(base + [100 + c, float(pr[0:3].sum()), float(pr[3:12].sum()), np.nan, np.nan,
                                            float(ac[0:3].sum()), float(ac[3:12].sum()), np.nan, np.nan])
                for c in CUTS:
                    pr, ac = preds[c][j, di, pi].numpy(), Y[i + j, di, pi]
                    rows.append(base + [c] + [float(pr[max(lo, c):hi].sum()) if hi > c else np.nan for lo, hi in BANDS]
                                + [float(ac[max(lo, c):hi].sum()) if hi > c else np.nan for lo, hi in BANDS])
    cols = ["trace", "target", "group", "day", "product", "decision"] + [f"pred_b{k}" for k in range(4)] + [f"act_b{k}" for k in range(4)]
    pd.DataFrame(rows, columns=cols).to_csv(out, index=False)
    print(f"{len(rows)} rows -> {out}")


if __name__ == "__main__":
    main()
