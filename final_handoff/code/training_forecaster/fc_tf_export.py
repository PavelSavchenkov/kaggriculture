"""Export a fc_tf.py transformer checkpoint (--model tf) for probe/fc_transformer.hpp.
Binary little-endian: int32 magic 0x46435446 ("FCTF"; "FCT2" 0x46435432 adds an int32 xlag flag and xlag mu, sd[24] after
lag sd), n_feat, n_lag, n_prod, width, layers, heads, n_out, max_pos, then float32:
feature mu[n_feat], sd[n_feat], lag mu[n_lag], sd[n_lag]; inp.weight[width][n_in], inp.bias; pos[max_pos][width];
per layer: norm1 w,b; in_proj_weight[3w][w], in_proj_bias; out_proj.weight[w][w], bias; norm2 w,b; linear1.weight[4w][w],
bias; linear2.weight[w][4w], bias; final norm w,b; out.weight[n_out][w], bias.
Also writes <out>.check: for 20 cached test sequences and each product, per day the model input vector (standardized features,
standardized lags, product one-hot) and PyTorch's 24 log-rates, for probe/fc_tf_check.
usage: fc_tf_export.py <checkpoint.pt> <cache dir> <out.bin>"""
import struct, sys
import numpy as np, torch
ck = torch.load(sys.argv[1], map_location="cpu", weights_only=False)
a = ck["args"]; st = ck["state"]; cache_dir = sys.argv[2]; out = sys.argv[3]
assert a["model"] == "tf", "only the per-product transformer is exported"
w, L, H = a["width"], a["layers"], a["heads"]
nf = len(ck["features"]); n_out = st["out.1.weight"].shape[0]; max_pos = st["pos"].shape[0]
f32 = lambda t: np.asarray(t, dtype=np.float32).tobytes()
with open(out, "wb") as f:
    xlag, stockin, xseen, nxt = int(a.get("xlag", 0)), int(a.get("stockin", 0)), int(a.get("xseen", 0)), int(a.get("next", 0))
    carryin = int(a.get("carryin", 0))
    if carryin: f.write(struct.pack("<14i", 0x46435436, nf, 24, 7, w, L, H, n_out, max_pos, xlag, stockin, xseen, nxt, carryin))  # "FCT6": + carryin
    elif nxt: f.write(struct.pack("<13i", 0x46435435, nf, 24, 7, w, L, H, n_out, max_pos, xlag, stockin, xseen, nxt))  # "FCT5": + next
    elif stockin or xseen: f.write(struct.pack("<12i", 0x46435434, nf, 24, 7, w, L, H, n_out, max_pos, xlag, stockin, xseen))  # "FCT4": + xlag, stockin, xseen
    elif xlag: f.write(struct.pack("<10i", 0x46435432, nf, 24, 7, w, L, H, n_out, max_pos, 1))  # "FCT2": + xlag mu, sd
    else: f.write(struct.pack("<9i", 0x46435446, nf, 24, 7, w, L, H, n_out, max_pos))
    for v in (ck["mu"], ck["sd"], ck["lag_mu"], ck["lag_sd"]) + ((ck["xlag_mu"], ck["xlag_sd"]) if xlag else ()): f.write(f32(v))
    f.write(f32(st["inp.weight"].numpy())); f.write(f32(st["inp.bias"].numpy())); f.write(f32(st["pos"].numpy()))
    for i in range(L):
        p = f"enc.layers.{i}."
        for k in ["norm1.weight", "norm1.bias", "self_attn.in_proj_weight", "self_attn.in_proj_bias", "self_attn.out_proj.weight",
                  "self_attn.out_proj.bias", "norm2.weight", "norm2.bias", "linear1.weight", "linear1.bias", "linear2.weight", "linear2.bias"]:
            f.write(f32(st[p + k].numpy()))
    for k in ["out.0.weight", "out.0.bias", "out.1.weight", "out.1.bias"]: f.write(f32(st[k].numpy()))
    if ck.get("intra"):  # fc_tf_intra.py: int32 width, then the 3 linear layers
        f.write(struct.pack("<i", st["intra.0.weight"].shape[0]))
        for k in ["intra.0.weight", "intra.0.bias", "intra.2.weight", "intra.2.bias", "intra.4.weight", "intra.4.bias"]: f.write(f32(st[k].numpy()))
open(out + ".txt", "w").write("features " + " ".join(ck["features"]) + "\ncounts " + " ".join(ck["counts"]) + "\nlog1p_positive money opp_money\n")

# PyTorch reference outputs on cached sequences
n_in = st["inp.weight"].shape[1]
class TF(torch.nn.Module):
    def __init__(s):
        super().__init__()
        s.inp = torch.nn.Linear(n_in, w); s.pos = torch.nn.Parameter(torch.zeros(max_pos, w))
        layer = torch.nn.TransformerEncoderLayer(w, H, w * 4, dropout=a["dropout"], batch_first=True, norm_first=True)
        s.enc = torch.nn.TransformerEncoder(layer, L); s.out = torch.nn.Sequential(torch.nn.LayerNorm(w), torch.nn.Linear(w, n_out))
        if ck.get("intra"):
            iw = st["intra.0.weight"].shape[0]
            s.intra = torch.nn.Sequential(torch.nn.Linear(w + 48 + 2 * int(a.get("stockin", 0)) + 2 * int(a.get("carryin", 0)) + 24 * int(a.get("xseen", 0)), iw), torch.nn.SiLU(), torch.nn.Linear(iw, iw), torch.nn.SiLU(), torch.nn.Linear(iw, 24 + int(a.get("next", 0))))
    def forward(s, x):  # hidden states
        Lq = x.shape[1]; mask = torch.triu(torch.full((Lq, Lq), float("-inf")), 1)
        return s.enc(s.inp(x) + s.pos[:Lq], mask=mask)
net = TF(); net.load_state_dict(st); net.eval()
z = np.load(f"{cache_dir}/seq_cache.npz", allow_pickle=True)
X, Lg, Y, split = z["X"], z["L"], z["Y"], z["split"].astype(str)
Sg = np.stack([z["S"], z["V"]], -1) if a.get("stockin", 0) else None  # log1p stock, visible per hour
Cg = np.stack([z["CA"], z["CE"]] + ([z["CN"], z["CF"]] if a.get("carryin", 0) >= 2 else []), -1) if a.get("carryin", 0) else None  # log1p carried beside the shed / elsewhere (/ near / far) per hour
idx = np.flatnonzero(split != "train")[:20]
Xn = (X[idx] - ck["mu"]) / ck["sd"]
Ln = (Lg[idx] - ck["lag_mu"]) / ck["lag_sd"] if a.get("lagnorm", 1) else Lg[idx]
with open(out + ".check", "w") as c:
    for j in range(len(idx)):
        for p in range(7):
            inp = np.concatenate([Xn[j, :, p], Ln[j, :, p], np.tile(np.eye(7)[p], (28, 1))], -1)
            if xlag: inp = np.concatenate([inp, (np.log1p(np.expm1(Lg[idx][j]).clip(0).sum(1)) - ck["xlag_mu"]) / ck["xlag_sd"]], -1)
            inp = inp.astype(np.float32)
            with torch.no_grad():
                hid = net(torch.tensor(inp)[None])[0]; lr = net.out(hid).numpy()
                if ck.get("intra"):  # per day a cut hour and the recorded sales before it -> intra-day log-rates
                    cut = (np.arange(28) * 7 + p) % 24
                    seen = (np.arange(24)[None] < cut[:, None]).astype(np.float32); ys = Y[idx][j, :, p]
                    zs = [hid, torch.tensor(np.log1p(ys) * seen), torch.tensor(seen)]
                    if Sg is not None: sv = np.expm1(Sg[idx][j, np.arange(28), p, cut]); zs.append(torch.tensor(np.log1p(sv), dtype=torch.float32))
                    if Cg is not None: cv = np.expm1(Cg[idx][j, np.arange(28), p, cut]); zs.append(torch.tensor(np.log1p(cv), dtype=torch.float32))
                    if xseen: zs.append(torch.tensor(np.log1p(Y[idx][j].sum(1)) * seen, dtype=torch.float32))  # all products
                    ilr = net.intra(torch.cat(zs, -1)).numpy()
            for d in range(28):
                line = " ".join(f"{v:.9g}" for v in inp[d]) + " | " + " ".join(f"{v:.9g}" for v in lr[d])
                if ck.get("intra"):
                    line += f" | {cut[d]} " + " ".join(f"{v:.9g}" for v in ys[d]) + (f" {sv[d, 0]:.9g} {sv[d, 1]:.9g}" if Sg is not None else "") + ("".join(f" {v:.9g}" for v in cv[d]) if Cg is not None else "") + ("".join(f" {v:.9g}" for v in Y[idx][j, d].sum(0)) if xseen else "") + " | " + " ".join(f"{v:.9g}" for v in ilr[d])
                c.write(line + "\n")
print("exported", out, "params", sum(v.numel() for v in st.values()))
