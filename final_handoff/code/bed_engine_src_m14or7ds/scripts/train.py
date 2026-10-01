"""Behavior cloning of DayIntent (designs/day_intent.md) from extracted arrays.

Architecture: MLP encoders for global state, crop groups and animal groups (width W),
mean/max pooling over existing groups (new-group sizes come from decoded counts, so
new groups are not pooled), a context MLP, and heads per field. Fixed values
(masks) get no loss and are never decoded. Strict FP32.

usage: train.py --arrays data/arrays_v1 --out models/<name> [--init models/<base>/model.pt]
       [--teams id,id] [--steps N]
"""
import argparse
import csv
import os
import glob
import json
import time
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F

GLOBAL, CROP, ANIMAL = 256, 48, 32
GRID = 24  # per-tile channels per farm (source/features.hpp grid_features)
CSTEPS, ASTEPS = 13, 3  # crop fields: 9 options, retain, clear, fertilize, harvest; animal: feed, care, collect
STYLE_BASE, STYLES = 224, 32  # teacher style one-hot in unused global slots (0 = unknown)
STAGE = 12  # decoded whole-farm fields fed to the group heads (grouped autoregression)
STAGE_SCALE = [20.0] * 5 + [5.0] * 6 + [1.0]
GF, CT, CM, AT, AM = 12, 14, 14, 4, 3
CLASSES = 101
CROP_FRESH, CROP_ONGOING, ANIMAL_FRESH = 14, 5, 7
torch.backends.cuda.matmul.allow_tf32 = False
torch.backends.cudnn.allow_tf32 = False


FIELDS = [("meta", np.int64, (8,), "i64"), ("global", np.float32, (GLOBAL,), "f32"), ("gtarget", np.int16, (GF,), "i16"),
          ("gmask", np.uint8, (GF,), "u8"), ("crop", np.float32, (CROP,), "f32"), ("ctarget", np.int16, (CT,), "i16"),
          ("cmask", np.uint8, (CM,), "u8"), ("animal", np.float32, (ANIMAL,), "f32"), ("atarget", np.int16, (AT,), "i16"),
          ("amask", np.uint8, (AM,), "u8")]


def load(arrays):
    """Comma-separated array directories, concatenated. Each field is preallocated and read in
    place, so peak memory is the data size (a list + concatenate would double it).
    "dir:w" samples that directory's dawns w times as often (d["row_weight"]) without loading it twice."""
    entries = [(e.split(":")[0], float(e.split(":")[1]) if ":" in e else 1.0) for e in arrays.split(",")]
    shards = [(p[:-len(".meta.i64")], w) for directory, w in entries for p in sorted(glob.glob(f"{directory}/shard_*.meta.i64"))]
    bases = [b for b, _ in shards]
    files = {name: ([(f"{b}.{name}.{suffix}", dtype) for b in bases], shape) for name, dtype, shape, suffix in FIELDS}
    grids = [next(((f"{b}.grid.{s}", t) for s, t in (("f16", np.float16), ("f32", np.float32)) if Path(f"{b}.grid.{s}").exists()), None)
             for b in bases]
    if all(grids):
        files["grid"] = (grids, (2, GRID, 10, 10))  # f16: converted to save memory
    d = {}
    for name, (entries, shape) in files.items():
        dtype, row = entries[0][1], int(np.prod(shape))
        counts = [Path(path).stat().st_size // (np.dtype(t).itemsize * row) for path, t in entries]
        out = np.empty((sum(counts), *shape), dtype=dtype)
        at = 0
        for (path, t), n in zip(entries, counts):
            if t == dtype:
                with open(path, "rb") as f:
                    f.readinto(memoryview(out[at:at + n]).cast("B"))
            else:
                out[at:at + n] = np.fromfile(path, dtype=t).reshape(n, *shape)
            at += n
        d[name] = out
        if name == "meta":
            d["row_weight"] = np.repeat([w for _, w in shards], counts)
    n_c, n_a = d["meta"][:, 6], d["meta"][:, 7]
    d["c_off"] = np.concatenate([[0], np.cumsum(n_c)])
    d["a_off"] = np.concatenate([[0], np.cumsum(n_a)])
    return d


CONDITION = 216  # global slots: strength known, strength / 200, days since 2026-08-15 / 40
LAND_IN = 215  # --land-cond: today's buy-land decision as an input (+1 buy, -1 no buy, 0 unknown); feature slot never written
GOAL = 219  # --goal: global slots 219-222 = known, own cows at dawn 6 / 4, own animals at dawn 14 / 20, 4 quadrants at dawn 11
# strength: submission final rating minus the ladder reference on its last date
# (scripts/build_conditions.py)


def condition_table(d, conditions):
    """Strength/recency inputs for every dawn (rows of d["meta"]); unknown strength gives zeros."""
    out = np.zeros((len(d["meta"]), 3), dtype=np.float32)
    keys = d["meta"][:, 0] * 2 + d["meta"][:, 1]
    unique, inverse = np.unique(keys, return_inverse=True)
    per = np.zeros((len(unique), 3), dtype=np.float32)
    for i, key in enumerate(unique):
        rating, day = conditions.get((int(key // 2), int(key % 2)), (None, None))
        if rating is not None:
            per[i, 0], per[i, 1] = 1.0, min(max(rating, -600.0), 300.0) / 200.0  # clipped: very weak submissions
        if day is not None:
            per[i, 2] = day / 40.0
    out[:] = per[inverse]
    return out


def goal_table(d, schema=1):
    """Hindsight goal inputs (--goal), written into d["global"] in place: what this teacher did later in the same game.
    Own animal counts are global slots 54-56 (goose, cow, sheep; / 10), own quadrants slot 6 (/ 4)."""
    g, meta = d["global"], d["meta"]
    keys, days = meta[:, 0] * 2 + meta[:, 1], meta[:, 2]
    unique = np.unique(keys)
    def at(day, value):
        rows = np.where(days == day)[0]
        out = np.full(len(unique), np.nan, dtype=np.float32)
        out[np.searchsorted(unique, keys[rows])] = value(rows)
        return out
    cows = at(6, lambda r: g[r, 55] * 10 / 4)
    land = at(11, lambda r: (g[r, 6] * 4 >= 3.5).astype(np.float32))
    if schema == 1:  # known, cows at dawn 6 / 4, animals at dawn 14 / 20, 4 quadrants by dawn 11
        animals = at(14, lambda r: g[r, 54:57].sum(1) * 10 / 20)
        per = np.stack([np.ones(len(unique), np.float32), cows, animals, land], 1)
    elif schema == 4:  # top-team plan (Sep 28 profile): known, cows at dawn 7 / 6, geese at dawn 12 / 8, strawberries planted
        # on days 2-3 / 6, quadrants at dawn 9 / 4 (top teams hold 2 cows at dawn 6 and buy ~4.5 on day 6)
        index = np.searchsorted(unique, keys)
        def planted(first, last, col):
            r = (days >= first) & (days <= last)
            return np.bincount(index[r], weights=d["gtarget"][r, col], minlength=len(unique)).astype(np.float32)
        per = np.stack([np.ones(len(unique), np.float32), at(7, lambda r: g[r, 55] * 10 / 6), at(12, lambda r: g[r, 54] * 10 / 8),
                        planted(2, 3, 3) / 6, at(9, lambda r: g[r, 6])], 1)
    elif schema == 5:  # schema 4 with the land slot replaced by melons planted on days 6-7 / 4 (the top teams' day-6 top-up to ~12
        # standing melons, Sep 28c meta: ~3-4 for DSM / DECEM / Vadim, ~0 for us; Weaknesses probe: +2 day-6 melons on v17 main +0.77k clean)
        index = np.searchsorted(unique, keys)
        def planted(first, last, col):
            r = (days >= first) & (days <= last)
            return np.bincount(index[r], weights=d["gtarget"][r, col], minlength=len(unique)).astype(np.float32)
        per = np.stack([np.ones(len(unique), np.float32), at(7, lambda r: g[r, 55] * 10 / 6), at(12, lambda r: g[r, 54] * 10 / 8),
                        planted(2, 3, 3) / 6, planted(6, 7, 4) / 4], 1)
    elif schema == 6:  # the Sep 28 evening gaps vs the top 5: known, geese at dawn 12 / 8, melons planted on days 6-7 / 4, wheat planted
        # on days 8-9 / 15 (our one-day lag), cash at dawn 10 / 1000 (slot 207 = money / 5000; our idle cash)
        index = np.searchsorted(unique, keys)
        def planted(first, last, col):
            r = (days >= first) & (days <= last)
            return np.bincount(index[r], weights=d["gtarget"][r, col], minlength=len(unique)).astype(np.float32)
        per = np.stack([np.ones(len(unique), np.float32), at(12, lambda r: g[r, 54] * 10 / 8), planted(6, 7, 4) / 4,
                        planted(8, 9, 0) / 15, at(10, lambda r: g[r, 207] * 5000 / 1000)], 1)
    elif schema == 7:  # the shared top-team plan (Weaknesses, sep28c: DSM / Vadim / DECEM / Goose): known, cows at dawn 5 / 5 (them
        # ~5.1, us ~4.0), cash at dawn 4 / 200 (them ~$15, us ~$196), melons planted on days 6-7 / 4, geese at dawn 12 / 8
        index = np.searchsorted(unique, keys)
        def planted(first, last, col):
            r = (days >= first) & (days <= last)
            return np.bincount(index[r], weights=d["gtarget"][r, col], minlength=len(unique)).astype(np.float32)
        per = np.stack([np.ones(len(unique), np.float32), at(5, lambda r: g[r, 55] * 10 / 5), at(4, lambda r: g[r, 207] * 5000 / 200),
                        planted(6, 7, 4) / 4, at(12, lambda r: g[r, 54] * 10 / 8)], 1)
    elif schema == 3:  # return conditioning: known, lead at dawn 29 / 20000 (slot 5 = (own - opp) / 50000), leading at dawn 29
        lead = at(29, lambda r: g[r, 5] * 50000 / 20000)
        per = np.stack([np.ones(len(unique), np.float32), lead, (lead > 0).astype(np.float32)], 1)
    else:  # 2: known, cows at dawn 6 / 4, geese at dawn 10 / 6, own cash at dawn 6 / 1000 (slot 207 = money / 5000), 4 quadrants by dawn 11
        geese = at(10, lambda r: g[r, 54] * 10 / 6)
        cash = at(6, lambda r: g[r, 207] * 5000 / 1000)
        per = np.stack([np.ones(len(unique), np.float32), cows, geese, cash, land], 1)
    per[np.isnan(per).any(1)] = 0  # a missing dawn: goal unknown
    width = per.shape[1]
    g[:, GOAL:GOAL + width] = per[np.searchsorted(unique, keys)]
    known = per[:, 0] > 0
    print(f"goals (schema {schema}): {int(known.sum())} of {len(unique)} perspectives; slot means " +
          " ".join(f"{v:.3f}" for v in per[known].mean(0)), flush=True)
    return width


def condition_inputs(rows, train):
    out = Batch.condition_rows[rows].copy()
    if train and Batch.condition_dropout:
        out[Batch.rng.random(len(rows)) < Batch.condition_dropout, :2] = 0
    return out


def gather(offsets, rows):
    """Concatenated index ranges offsets[r]..offsets[r+1] for the given rows."""
    lengths = offsets[rows + 1] - offsets[rows]
    starts = np.repeat(offsets[rows] - np.cumsum(lengths) + lengths, lengths)
    return starts + np.arange(lengths.sum())


class Batch:
    style_of = None      # team id -> style index (data/styles.json) when style conditioning is on
    style_dropout = 0.0  # share of training dawns shown as "unknown" style
    sites = False        # arrays carry site-capacity features (v4): mask new-entity totals
    conditions = None    # (episode, seat) -> (strength, day index) for strength/recency conditioning
    condition_rows = None  # the same per dawn (condition_table)
    condition_dropout = 0.0
    augment = False      # random dihedral grid transforms in training
    style_v2 = False     # unknown teams -> style 31, dropout -> no style
    search_opening = None  # (days, style): search-game rows use this style before `days`, none after
    goal_dropout = 0.0   # --goal: share of training dawns shown with unknown goals
    land_cond = None     # --land-cond: share of training dawns shown with an unknown land decision (None: off)
    land_scale = 1.0     # --land-scale: input value of a known decision (+-scale; Adam moves the zero-initialized column scale x faster)
    goal_width = 4       # goal slots from GOAL (schema 1: 4, schema 2: 5)
    rng = np.random.default_rng(1)

    def __init__(self, d, rows, device, train=False):
        cs = gather(d["c_off"], rows)
        as_ = gather(d["a_off"], rows)
        t = lambda x, dt=torch.float32: torch.as_tensor(x, dtype=dt, device=device)
        self.n = len(rows)
        g = d["global"][rows].copy()
        if Batch.style_of is not None:
            # v1: unknown teams and dropout share index 0. v2 (--style-v2): unknown teams get
            # index 31 ("other") and dropout gives no style at all (the pooled input).
            unknown = 31 if Batch.style_v2 else 0
            styles = np.array([Batch.style_of.get(int(team), unknown) for team in d["meta"][rows, 3]])
            if Batch.search_opening is not None:  # search games (synthetic teams): the agent's inference styles
                days, style = Batch.search_opening
                search = d["meta"][rows, 3] >= 3_000_000_000
                styles[search] = np.where(d["meta"][rows, 2][search] < days, style, -1)
            if train and Batch.style_dropout:
                styles[Batch.rng.random(len(styles)) < Batch.style_dropout] = -1 if Batch.style_v2 else 0
            keep = styles >= 0
            g[np.arange(len(rows))[keep], STYLE_BASE + styles[keep]] = 1.0
        if Batch.conditions is not None:
            g[:, CONDITION:CONDITION + 3] = condition_inputs(rows, train)
        if train and Batch.goal_dropout:
            g[Batch.rng.random(len(rows)) < Batch.goal_dropout, GOAL:GOAL + Batch.goal_width] = 0
        if Batch.land_cond is not None:
            known = d["gmask"][rows, 11] == 0  # gmask 1: label masked out
            if train:
                known &= Batch.rng.random(len(rows)) >= Batch.land_cond
            g[:, LAND_IN] = np.where(known, (2.0 * d["gtarget"][rows, 11] - 1) * Batch.land_scale, 0)
        self.g = t(g)
        self.gt = t(d["gtarget"][rows], torch.long)
        self.gm = t(d["gmask"][rows], torch.bool)
        self.c = t(d["crop"][cs])
        self.ct = t(d["ctarget"][cs])
        self.cm = t(d["cmask"][cs], torch.bool)
        self.ci = t(np.repeat(np.arange(len(rows)), d["meta"][rows, 6]), torch.long)
        self.a = t(d["animal"][as_])
        self.at = t(d["atarget"][as_])
        self.am = t(d["amask"][as_], torch.bool)
        self.ai = t(np.repeat(np.arange(len(rows)), d["meta"][rows, 7]), torch.long)
        grid = d["grid"][rows].astype(np.float32) if "grid" in d else None
        if grid is not None and train and Batch.augment:
            # Random dihedral transform per dawn (both farms): the shed is central and
            # DayIntent labels are counts, so rotations and flips keep every label.
            k = Batch.rng.integers(8, size=len(rows))
            for tr in range(1, 8):
                idx = np.where(k == tr)[0]
                if len(idx):
                    g2 = np.rot90(grid[idx], tr % 4, axes=(3, 4))
                    grid[idx] = g2[..., ::-1] if tr >= 4 else g2
        self.grid = t(grid) if grid is not None else None


class GpuData:
    """The arrays resident on the GPU (--gpu-data): batches are gathered on the device, so training
    does not wait for the CPU. Compact dtypes stay compact and are converted per batch."""

    def __init__(self, d, device, host_grid=False):
        t = lambda x: torch.as_tensor(np.ascontiguousarray(x), device=device)
        self.device = device
        self.fields = {k: t(d[k]) for k in ("meta", "global", "gtarget", "gmask", "crop", "ctarget", "cmask",
                                            "animal", "atarget", "amask", "c_off", "a_off")}
        # --host-grid: the tile grids (the largest field) stay in host memory; each batch's rows are copied over
        self.host_grid = torch.from_numpy(d["grid"]) if host_grid else None
        if not host_grid:
            self.fields["grid"] = t(d["grid"])
        n = len(d["meta"])
        styles = np.full(n, -1, dtype=np.int64)  # per-dawn style index before dropout (-1: none)
        if Batch.style_of is not None:
            unknown = 31 if Batch.style_v2 else 0
            styles = np.array([Batch.style_of.get(int(team), unknown) for team in d["meta"][:, 3]], dtype=np.int64)
            if Batch.search_opening is not None:
                days, style = Batch.search_opening
                search = d["meta"][:, 3] >= 3_000_000_000
                styles[search] = np.where(d["meta"][search, 2] < days, style, -1)
        self.styles = t(styles)
        self.conditions = t(Batch.condition_rows) if Batch.condition_rows is not None else None


def gpu_batch(gd, rows, train=False):
    """The same fields as Batch, gathered on the GPU."""
    f, dev = gd.fields, gd.device
    host_rows = torch.as_tensor(rows, dtype=torch.long)
    rows = host_rows.to(dev)
    b = type("GpuBatch", (), {})()
    b.n = len(rows)
    g = f["global"][rows].clone()
    if Batch.style_of is not None:
        styles = gd.styles[rows].clone()
        if train and Batch.style_dropout:
            styles[torch.rand(b.n, device=dev) < Batch.style_dropout] = -1 if Batch.style_v2 else 0
        keep = styles >= 0
        g[torch.arange(b.n, device=dev)[keep], STYLE_BASE + styles[keep]] = 1.0
    if gd.conditions is not None:
        cond = gd.conditions[rows].clone()
        if train and Batch.condition_dropout:
            cond[torch.rand(b.n, device=dev) < Batch.condition_dropout, :2] = 0
        g[:, CONDITION:CONDITION + 3] = cond
    if train and Batch.goal_dropout:
        g[torch.rand(b.n, device=dev) < Batch.goal_dropout, GOAL:GOAL + Batch.goal_width] = 0
    b.g = g
    b.gt = f["gtarget"][rows].long()
    b.gm = f["gmask"][rows].bool()
    if Batch.land_cond is not None:
        known = ~b.gm[:, 11]  # gmask 1: label masked out
        if train:
            known = known & (torch.rand(b.n, device=dev) >= Batch.land_cond)
        g[:, LAND_IN] = torch.where(known, (2.0 * b.gt[:, 11].float() - 1) * Batch.land_scale, torch.zeros_like(g[:, 0]))

    def members(offsets):
        start, length = offsets[rows], offsets[rows + 1] - offsets[rows]
        owner = torch.repeat_interleave(torch.arange(b.n, device=dev), length)
        first = torch.repeat_interleave(torch.cumsum(length, 0) - length, length)
        return torch.repeat_interleave(start, length) + torch.arange(len(owner), device=dev) - first, owner

    cs, b.ci = members(f["c_off"])
    as_, b.ai = members(f["a_off"])
    b.c, b.ct, b.cm = f["crop"][cs], f["ctarget"][cs].float(), f["cmask"][cs].bool()
    b.a, b.at, b.am = f["animal"][as_], f["atarget"][as_].float(), f["amask"][as_].bool()
    b.grid = (gd.host_grid[host_rows].to(dev) if gd.host_grid is not None else f["grid"][rows]).float()
    return b


def mlp(i, w, o):
    return nn.Sequential(nn.Linear(i, w), nn.ReLU(), nn.Linear(w, w), nn.ReLU(), nn.Linear(w, o))


def pool(x, index, keep, n):
    """Mean, max and sum/10 over kept rows per sample (zeros for none)."""
    x = x * keep[:, None]
    total = torch.zeros(n, x.shape[1], device=x.device).index_add_(0, index, x)
    count = torch.zeros(n, device=x.device).index_add_(0, index, keep).clamp(min=1)
    top = torch.full((n, x.shape[1]), -1e9, device=x.device)
    top = top.index_reduce_(0, index, torch.where(keep[:, None] > 0, x, torch.full_like(x, -1e9)), "amax")
    top = torch.where(top < -1e8, torch.zeros_like(top), top)
    return torch.cat([total / count[:, None], top, total / 10.0], 1)


class GridNet(nn.Module):
    """Shared 3x3 CNN over one farm's 10x10 tile channels; mean and max pooled."""

    def __init__(self, channels=32):
        super().__init__()
        self.conv1 = nn.Conv2d(GRID, channels, 3, padding=1)
        self.conv2 = nn.Conv2d(channels, channels, 3, padding=1)

    def forward(self, x):
        h = F.relu(self.conv2(F.relu(self.conv1(x))))
        return torch.cat([h.mean((2, 3)), h.amax((2, 3))], 1)


class Net(nn.Module):
    def __init__(self, w=128, grid=False, marginal=False):
        super().__init__()
        self.marginal = marginal
        self.g_enc, self.c_enc, self.a_enc = mlp(GLOBAL, w, w), mlp(CROP, w, w), mlp(ANIMAL, w, w)
        self.ctx = mlp(7 * w + (128 if grid else 0), w, w)
        self.g_head = nn.Linear(w, 11 * CLASSES + 1 + CLASSES + 5 + CLASSES + 3)
        # Sequential count decoders (as the Sep 23 BC): one call per field, conditioned on
        # the field, the values already decided in the group, its size and legal maximum.
        if marginal:  # one count distribution per field over 0..size, for MAP (DP) decoding
            self.c_head = mlp(2 * w + STAGE, w, CSTEPS * CLASSES)
            self.a_head = mlp(2 * w + STAGE, w, ASTEPS * CLASSES)
        else:
            self.c_head = mlp(2 * w + STAGE + 2 * CSTEPS + 2, w, CLASSES)
            self.a_head = mlp(2 * w + STAGE + 2 * ASTEPS + 2, w, CLASSES)
        self.grid = GridNet() if grid else None

    def forward(self, b):
        g = F.relu(self.g_enc(b.g))
        c = F.relu(self.c_enc(b.c))
        a = F.relu(self.a_enc(b.a))
        keep_c = 1.0 - b.c[:, CROP_FRESH]
        keep_a = 1.0 - b.a[:, ANIMAL_FRESH]
        parts = [g, pool(c, b.ci, keep_c, b.n), pool(a, b.ai, keep_a, b.n)]
        if self.grid is not None:
            parts += [self.grid(b.grid[:, 0]), self.grid(b.grid[:, 1])]
        ctx = F.relu(self.ctx(torch.cat(parts, 1)))
        out_g = self.g_head(ctx)
        # Stage A (whole-farm decisions) conditions the group heads; teacher-forced labels
        # in training, decoded values at inference.
        stage = b.gt[:, :STAGE].float() / torch.tensor(STAGE_SCALE, device=b.g.device)
        out_c = torch.cat([c, ctx[b.ci], stage[b.ci]], 1)  # per-group base; steps are added in losses
        out_a = torch.cat([a, ctx[b.ai], stage[b.ai]], 1)
        return out_g, out_c, out_a


NEG = -1e9


def losses(net, b):
    og, oc, oa = net(b)  # oc / oa: per-group decoder bases
    parts = {}
    # Whole-farm counts: 101-way classification per field; land: binary.
    counts = og[:, :11 * CLASSES].reshape(-1, 11, CLASSES)
    ce = F.cross_entropy(counts.reshape(-1, CLASSES), b.gt[:, :11].reshape(-1), reduction="none").reshape(-1, 11)
    live = (~b.gm[:, :11]).float()
    parts["global_counts"] = (ce * live).sum() / b.n
    land = og[:, 11 * CLASSES]
    parts["land"] = (F.binary_cross_entropy_with_logits(land, b.gt[:, 11].float(), reduction="none")
                     * (~b.gm[:, 11]).float()).sum() / b.n
    # Factorized new entities: total count (101-way) and type shares (members as samples).
    base = 11 * CLASSES + 1
    for name, lo, k in (("crop", 0, 5), ("animal", 5, 3)):
        live = (~b.gm[:, lo]).float()
        counts = b.gt[:, lo:lo + k].float()
        total = counts.sum(1)
        logits_total = og[:, base:base + CLASSES]
        if Batch.sites:  # at most the sites possible today (features v4, global slot 214)
            most = (b.g[:, 214] * 50).round().long()
            logits_total = logits_total.masked_fill(torch.arange(CLASSES, device=og.device)[None, :] > most[:, None], NEG)
        parts[f"{name}_total"] = (F.cross_entropy(logits_total, total.long().clamp(max=CLASSES - 1), reduction="none") * live).sum() / b.n
        shares = F.log_softmax(og[:, base + CLASSES:base + CLASSES + k], 1)
        parts[f"{name}_share"] = -((shares * counts).sum(1) * live).sum() / b.n
        base += CLASSES + k
    # Group fields: a categorical over the concrete count 0..100 per field, masked to the
    # legal range given earlier fields (teacher-forced), CE on the exact label count.
    # Crop head: 9 one-shot options, retain, clear, fertilize, harvest; animal: feed, care, collect.
    size = b.ct[:, 0].long()
    ongoing = b.c[:, CROP_ONGOING] > 0.5
    k = torch.arange(CLASSES, device=b.g.device)
    zero = torch.zeros_like(size)

    def count_loss(logits, label, upper, active):
        """logits [n,101]; legal counts 0..upper; loss on active rows."""
        illegal = k[None, :] > upper[:, None]
        loss = F.cross_entropy(logits.masked_fill(illegal, NEG), label.clamp(min=0), reduction="none")
        return (loss * active.float()).sum() / b.n

    def steps(base, values, uppers, head, n_steps):
        """Teacher-forced step inputs: base, field one-hot, earlier values, size, upper."""
        g = base.shape[0]
        onehot = torch.eye(n_steps, device=base.device)[None].expand(g, -1, -1)
        earlier = torch.tril(torch.ones(n_steps, n_steps, device=base.device), -1)
        prev = values.float()[:, None, :] * earlier[None] / 100.0
        extra = torch.stack([sizes_of(values, n_steps), uppers.float()], 2) / 100.0
        x = torch.cat([base[:, None, :].expand(-1, n_steps, -1), onehot, prev, extra], 2)
        return head(x.reshape(g * n_steps, -1)).reshape(g, n_steps, CLASSES)

    one = (~ongoing) & (size > 0)
    existing = ongoing & (size > 0) & (b.c[:, CROP_FRESH] < 0.5)
    values = b.ct[:, 1:14].long()
    retain = values[:, 9]
    option_upper = size[:, None] - torch.cumsum(values[:, :9], 1) + values[:, :9]  # size - earlier options
    uppers = torch.cat([option_upper, torch.where(b.cm[:, 9], zero, size)[:, None], (size - retain)[:, None],
                        retain[:, None], size[:, None]], 1)
    sizes_of = lambda v, n: size[:, None].float().expand(-1, n)
    if net.marginal:  # marginals: every field over 0..size (retain over 0 when fixed)
        uppers = torch.cat([size[:, None].expand(-1, 9), uppers[:, 9:10], size[:, None].expand(-1, 3)], 1)
        logits = net.c_head(oc).reshape(-1, CSTEPS, CLASSES)
    else:
        logits = steps(oc, values, uppers, net.c_head, CSTEPS)
    can_abandon = ~b.cm[:, 11]
    active = torch.cat([one[:, None] & ~b.cm[:, :9], (existing & ~b.cm[:, 9])[:, None],
                        (existing & ~b.cm[:, 10] & can_abandon)[:, None], (existing & ~b.cm[:, 12])[:, None],
                        (existing & ~b.cm[:, 13])[:, None]], 1)
    names = ["options"] * 9 + ["retain", "clear", "fertilize", "harvest"]
    for j in range(CSTEPS):
        parts[names[j]] = parts.get(names[j], 0) + count_loss(logits[:, j], values[:, j], uppers[:, j], active[:, j])
    asize = b.at[:, 0].long()
    avalues = b.at[:, 1:4].long()
    feed = avalues[:, 0]
    auppers = torch.stack([asize, feed, asize], 1)
    sizes_of = lambda v, n: asize[:, None].float().expand(-1, n)
    if net.marginal:
        auppers = asize[:, None].expand(-1, 3)
        alogits = net.a_head(oa).reshape(-1, ASTEPS, CLASSES)
    else:
        alogits = steps(oa, avalues, auppers, net.a_head, ASTEPS)
    live = asize > 0
    aactive = torch.stack([live & ~b.am[:, 0], live & ~b.am[:, 1] & (feed > 0), live & ~b.am[:, 2]], 1)
    for j, name in enumerate(["feed", "care", "collect"]):
        parts[name] = count_loss(alogits[:, j], avalues[:, j], auppers[:, j], aactive[:, j])
    return parts


@torch.no_grad()
def floor(d, rows, device):
    """Lowest achievable loss: entropy of the soft targets (hard targets contribute 0)."""
    b = Batch(d, rows, device)
    h = lambda p: -(p * torch.log(p.clamp(min=1e-12)))
    size = b.ct[:, 0]
    ongoing = b.c[:, CROP_ONGOING] > 0.5
    one = (~ongoing) & (size > 0)
    total = 0.0
    p = b.ct[:, 1:10] / size.clamp(min=1)[:, None]
    total += float((h(p).sum(1) * size)[one].sum())
    abandon = size - b.ct[:, 10] - b.ct[:, 11]
    pt = torch.stack([b.ct[:, 10], b.ct[:, 11], abandon], 1) / size.clamp(min=1)[:, None]
    used = ongoing & (~b.cm[:, 9:12]).any(1) & (size > 0)
    total += float((h(pt).sum(1) * size)[used].sum())
    def binary(pos, n, mask):
        q = (pos / n.clamp(min=1)).clamp(0, 1)
        ok = (~mask) & (n > 0)
        return float(((h(q) + h(1 - q)) * n)[ok].sum())
    total += binary(b.ct[:, 12], b.ct[:, 10], b.cm[:, 12] | ~ongoing)
    total += binary(b.ct[:, 13], size, b.cm[:, 13] | ~ongoing)
    total += binary(b.at[:, 1], b.at[:, 0], b.am[:, 0]) + binary(b.at[:, 2], b.at[:, 1], b.am[:, 1])
    total += binary(b.at[:, 3], b.at[:, 0], b.am[:, 2])
    for lo, k in ((0, 5), (5, 3)):
        counts = b.gt[:, lo:lo + k].float()
        n = counts.sum(1)
        live = ~b.gm[:, lo]
        total += float((h(counts / n.clamp(min=1)[:, None]).sum(1) * n)[live].sum())
    return total / len(rows)


@torch.no_grad()
def evaluate(net, d, rows, device, batch=2048, gd=None):
    net.eval()
    totals = {}
    for start in range(0, len(rows), batch):
        b = gpu_batch(gd, rows[start:start + batch]) if gd else Batch(d, rows[start:start + batch], device)
        for k, v in losses(net, b).items():
            totals[k] = totals.get(k, 0.0) + float(v) * b.n
    net.train()
    return {k: v / len(rows) for k, v in totals.items()}


def export(net, path):
    """Raw FP32 weights in module order: [rows, cols, weights..., bias...] per Linear."""
    layers = [m for m in net.modules() if isinstance(m, nn.Linear)]
    convs = [m for m in net.modules() if isinstance(m, nn.Conv2d)]
    with open(path, "wb") as f:
        if convs:  # negative count: a conv section precedes the linear layers
            np.array([-len(convs)], dtype=np.int32).tofile(f)
            for m in convs:
                np.array(m.weight.shape[:3], dtype=np.int32).tofile(f)
                m.weight.detach().cpu().numpy().astype(np.float32).tofile(f)
                m.bias.detach().cpu().numpy().astype(np.float32).tofile(f)
        np.array([len(layers)], dtype=np.int32).tofile(f)
        for m in layers:
            np.array(m.weight.shape, dtype=np.int32).tofile(f)
            m.weight.detach().cpu().numpy().astype(np.float32).tofile(f)
            m.bias.detach().cpu().numpy().astype(np.float32).tofile(f)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--arrays", default="data/arrays_v1")
    p.add_argument("--out", required=True)
    p.add_argument("--init")
    p.add_argument("--teams", default="")
    p.add_argument("--steps", type=int, default=6000)
    p.add_argument("--batch", type=int, default=512)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--width", type=int, default=128)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--overfit", type=int, default=0, help="train and evaluate on this many train dawns")
    p.add_argument("--grid", action="store_true", help="add the tile CNN (arrays must contain grids)")
    p.add_argument("--marginal", action="store_true", help="independent count heads (decode by exact MAP DP)")
    p.add_argument("--style", action="store_true", help="condition on teacher team (data/styles.json)")
    p.add_argument("--style-dropout", type=float, default=0.1)
    p.add_argument("--style-v2", action="store_true", help="unknown teams: style 31; dropout: no style (pooled)")
    p.add_argument("--search-opening", help="'days style': search-game rows (team >= 3e9) as the agent plays them")
    p.add_argument("--gpu-data", action="store_true", help="keep the arrays on the GPU (batches gathered on the device)")
    p.add_argument("--land-cond", type=float, default=None,
                   help="today's buy-land decision as input slot 215 with this dropout (zero-initialized column; sidecar model.bin.landcond)")
    p.add_argument("--land-scale", type=float, default=1.0, help="--land-cond input value of a known decision (+-scale)")
    p.add_argument("--host-grid", action="store_true", help="with --gpu-data: keep the tile grids in host memory")
    p.add_argument("--condition", help="rating/recency conditioning table (data/conditions_v5.csv)")
    p.add_argument("--condition-dropout", type=float, default=0.1)
    p.add_argument("--augment", action="store_true", help="random dihedral transforms of the farm grids")
    p.add_argument("--grid-dropout", type=float, default=0.0, help="share of training dawns with the whole grid input zeroed "
                   "(inference: <model>.gridoff zeroes it on chosen days)")
    p.add_argument("--goal", action="store_true", help="hindsight goal inputs at global slots 219-222 (goal_table)")
    p.add_argument("--goal-dropout", type=float, default=0.3)
    p.add_argument("--goal-schema", type=int, default=1, help="1: cows6 / animals14 / q4; 2: cows6 / geese10 / cash6 / q4; 3: lead29 / leading29; 4: cows7 / geese12 / straw d2-3 / quadrants9; 5: 4 with melons planted d6-7 instead of quadrants9; 6: geese12 / melons d6-7 / wheat d8-9 / cash10; 7: cows5 / cash4 / melons d6-7 / geese12")
    p.add_argument("--goal-adapter", action="store_true", help="train only the first global layer's goal columns (219-222, zero-initialized); "
                   "everything else stays as --init, so unknown goals reproduce the initial network exactly")
    # Training-data filters (validation rows are never filtered, so losses stay comparable).
    p.add_argument("--min-strength", type=float, help="keep perspectives with known strength >= this")
    p.add_argument("--since-day", type=int, help="keep replays from this day index (days since 2026-08-15)")
    p.add_argument("--winners", action="store_true", help="keep perspectives that won their episode")
    p.add_argument("--strength-temp", type=float, help="sample training dawns with probability proportional to "
                   "exp(strength / T) (strength from --condition; unknown: the median)")
    p.add_argument("--denial-temp", type=float, help="sample training dawns with weight exp(team denial / T), team denial from "
                   "data/team_denial.csv (teams with >= 40 perspectives; others 0)")
    p.add_argument("--margin-temp", type=float, help="sample training dawns with probability proportional to "
                   "sigmoid(margin / T) of their episode (data/perspectives_meta.csv; unknown: 0.5)")
    p.add_argument("--condition-strength", type=float, default=150.0, help="strength the exported model is asked to play at")
    p.add_argument("--valid-cap", type=int, default=30000, help="fixed random subset of validation dawns")
    p.add_argument("--condition-day", type=float, default=41.0, help="day index the exported model is asked to play at")
    args = p.parse_args()
    torch.set_num_threads(int(os.environ.get("TORCH_THREADS", "4")))  # shared machine: cap the CPU thread pool
    torch.manual_seed(args.seed)
    rng = np.random.default_rng(args.seed)
    device = "cuda" if torch.cuda.is_available() else "cpu"
    if args.style:
        Batch.style_of = {int(k): v for k, v in json.load(open("data/styles.json"))["styles"].items()}
        Batch.style_dropout = args.style_dropout
        Batch.style_v2 = args.style_v2
        if args.search_opening:
            Batch.search_opening = tuple(int(x) for x in args.search_opening.split())
    if args.condition:
        Batch.conditions = {}
        for path in args.condition.split(","):  # comma-separated tables are merged
            with open(path) as f:
                for r in csv.DictReader(f):
                    Batch.conditions[(int(r["episode"]), int(r["seat"]))] = (
                        float(r["rating"]) if r["rating"] else None, float(r["day_index"]) if r["day_index"] else None)
        Batch.condition_dropout = args.condition_dropout
    Batch.augment = args.augment
    d = load(args.arrays)
    if args.goal:
        Batch.goal_width = goal_table(d, args.goal_schema)
        Batch.goal_dropout = args.goal_dropout
    Batch.land_cond, Batch.land_scale = args.land_cond, args.land_scale
    if Batch.conditions is not None:
        Batch.condition_rows = condition_table(d, Batch.conditions)
    # Feature version of the arrays: v4 adds site capacity at global slots 208-214.
    features = 4 if np.abs(d["global"][:, 208:215]).max() > 0 else 3
    Batch.sites = features >= 4
    split, team = d["meta"][:, 5], d["meta"][:, 3]
    train = np.where(split == 0)[0]
    valid = np.where(split == 1)[0]
    if len(valid) > args.valid_cap:
        valid = np.sort(np.random.default_rng(12345).choice(valid, args.valid_cap, replace=False))
    if args.teams:
        chosen = [int(x) for x in args.teams.split(",")]
        train, valid = train[np.isin(team[train], chosen)], valid[np.isin(team[valid], chosen)]
    if args.min_strength is not None or args.since_day is not None or args.winners:
        table = {}
        with open("data/conditions_v5.csv") as f:
            for r in csv.DictReader(f):
                table[(int(r["episode"]), int(r["seat"]))] = [float(r["rating"]) if r["rating"] else None,
                                                             int(r["day_index"]) if r["day_index"] else None, None]
        with open("data/perspectives_meta.csv") as f:
            for r in csv.DictReader(f):
                key = (int(r["episode"]), int(r["seat"]))
                if key in table and r["margin"]:
                    table[key][2] = float(r["margin"])

        def keep(row):
            strength, day, margin = table.get((int(d["meta"][row, 0]), int(d["meta"][row, 1])), (None, None, None))
            if args.min_strength is not None and (strength is None or strength < args.min_strength):
                return False
            if args.since_day is not None and (day is None or day < args.since_day):
                return False
            return not args.winners or (margin is not None and margin > 0)

        before = len(train)
        # One decision per perspective (all its days share it).
        perspective = {}
        mask = np.zeros(len(train), dtype=bool)
        for i, row in enumerate(train):
            key = (int(d["meta"][row, 0]), int(d["meta"][row, 1]))
            if key not in perspective:
                perspective[key] = keep(row)
            mask[i] = perspective[key]
        train = train[mask]
        print(f"filters kept {len(train)} of {before} train dawns", flush=True)
    if args.overfit:
        train = valid = np.sort(rng.choice(train, args.overfit, replace=False))
        print(f"overfit check on {len(train)} train dawns; loss floor {floor(d, train, device):.3f}", flush=True)
    print(f"train {len(train)} valid {len(valid)} dawns", flush=True)
    probs = None
    if (d["row_weight"] != 1).any():
        probs = d["row_weight"][train] / d["row_weight"][train].sum()
        print(f"directory-weighted sampling: effective dawns {1 / (probs ** 2).sum():.0f}", flush=True)
    if args.strength_temp:
        if Batch.condition_rows is None:
            raise SystemExit("--strength-temp needs --condition")
        known = Batch.condition_rows[train, 0] > 0
        strength = Batch.condition_rows[train, 1] * 200.0
        strength = np.where(known, strength, np.median(strength[known]))
        weights = np.exp((strength - strength.max()) / args.strength_temp)
        probs = weights / weights.sum() if probs is None else probs * weights / (probs * weights).sum()
        print(f"strength-weighted sampling: T {args.strength_temp}, effective dawns {1 / (probs ** 2).sum():.0f}", flush=True)
    if args.margin_temp:
        margins = {}
        for path in ("data/perspectives_meta.csv", "data/perspectives_margin_fresh.csv"):  # fresh pulls: final money from the traces
            with open(path) as f:
                for r in csv.DictReader(f):
                    if r["margin"]:
                        margins[(int(r["episode"]), int(r["seat"]))] = float(r["margin"])
        keys = d["meta"][train, 0] * 2 + d["meta"][train, 1]
        unique, inverse = np.unique(keys, return_inverse=True)
        per = np.array([margins.get((int(k // 2), int(k % 2)), 0.0) for k in unique])
        print(f"margin known for {sum((int(k // 2), int(k % 2)) in margins for k in unique)} of {len(unique)} perspectives", flush=True)
        weights = (1 / (1 + np.exp(-per / args.margin_temp)))[inverse]
        probs = weights / weights.sum() if probs is None else probs * weights / (probs * weights).sum()
        print(f"margin-weighted sampling: T {args.margin_temp}, effective dawns {1 / (probs ** 2).sum():.0f}", flush=True)
    if args.denial_temp:  # team-level denial: how far the team pushes opponents below their typical money (scripts/denial_table.py)
        denial = {}
        with open("data/team_denial.csv") as f:
            for r in csv.DictReader(f):
                if int(r["n"]) >= 40:
                    denial[int(r["team"])] = float(r["denial"])
        team_of = d["meta"][train, 3]
        per = np.array([denial.get(int(t), 0.0) for t in team_of])
        print(f"denial known for {np.mean([int(t) in denial for t in team_of]):.0%} of training dawns", flush=True)
        weights = np.exp(np.clip(per / args.denial_temp, -5, 5))
        probs = weights / weights.sum() if probs is None else probs * weights / (probs * weights).sum()
        print(f"denial-weighted sampling: T {args.denial_temp}, effective dawns {1 / (probs ** 2).sum():.0f}", flush=True)
    net = Net(args.width, args.grid, args.marginal).to(device)
    if args.init:
        net.load_state_dict(torch.load(args.init, map_location=device))
    if args.land_cond is not None:  # the new input starts with no effect: unknown (0) and known begin as the init network
        with torch.no_grad():
            net.g_enc[0].weight[:, LAND_IN] = 0
    if args.goal_adapter:  # only the goal columns of the first global layer learn
        first = net.g_enc[0].weight
        with torch.no_grad():
            first[:, GOAL:GOAL + Batch.goal_width] = 0
        for q in net.parameters():
            q.requires_grad_(False)
        first.requires_grad_(True)
        mask = torch.zeros_like(first)
        mask[:, GOAL:GOAL + Batch.goal_width] = 1
        first.register_hook(lambda grad: grad * mask)
        opt = torch.optim.AdamW([first], lr=args.lr, weight_decay=0.0)
    else:
        opt = torch.optim.AdamW(net.parameters(), lr=args.lr, weight_decay=1e-4)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, args.steps)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    history, best, t0 = [], float("inf"), time.time()
    gd = None
    if args.gpu_data:
        if Batch.augment:
            raise SystemExit("--gpu-data does not support --augment")
        gd = GpuData(d, device, args.host_grid)
        for k in ("global", "crop", "animal", "ctarget", "cmask", "atarget", "amask") + (() if args.host_grid else ("grid",)):
            d.pop(k)  # host copies are no longer needed
    cdf = None if probs is None else np.cumsum(probs) / probs.sum()  # rng.choice(p=) rebuilds this every call
    for step in range(1, args.steps + 1):
        rows = rng.choice(train, args.batch) if cdf is None else train[np.minimum(np.searchsorted(cdf, rng.random(args.batch)), len(train) - 1)]
        b = gpu_batch(gd, rows, train=True) if gd else Batch(d, rows, device, train=True)
        if args.grid_dropout and b.grid is not None:  # both farms' grids zeroed: plans that do not hinge on the tile layout
            b.grid = b.grid * (torch.rand(b.n, device=b.grid.device) >= args.grid_dropout).to(b.grid.dtype)[:, None, None, None, None]
        parts = losses(net, b)
        loss = sum(parts.values())
        opt.zero_grad()
        loss.backward()
        nn.utils.clip_grad_norm_(net.parameters(), 1.0)
        opt.step()
        sched.step()
        if step % 500 == 0 or step == args.steps:
            v = evaluate(net, d, valid, device, gd=gd)
            total = sum(v.values())
            history.append({"step": step, "train": float(loss), "valid": total, **v})
            print(f"step {step} train {float(loss):.3f} valid {total:.3f} "
                  + " ".join(f"{k} {x:.3f}" for k, x in v.items()) + f" {time.time() - t0:.0f}s", flush=True)
            torch.save(net.state_dict(), out / f"step{step}.pt")
            if total < best:
                best = total
                torch.save(net.state_dict(), out / "model.pt")
                export(net, out / "model.bin")
                (out / "model.bin.features").write_text(f"{features}\n")
                if args.style and not args.style_v2:  # default style: 0 (v1 pooled / unknown team)
                    (out / "model.bin.style").write_text("0\n")
                if args.land_cond is not None:  # the decoder writes slot LAND_IN only for models with this sidecar
                    (out / "model.bin.landcond").write_text(f"{args.land_scale:g}\n")
                if args.condition:  # inference inputs at slots CONDITION..+2
                    (out / "model.bin.condition").write_text(
                        f"1 {args.condition_strength / 200:.4f} {args.condition_day / 40:.4f}\n")
    (out / "history.json").write_text(json.dumps({"args": vars(args), "history": history}, indent=1))


if __name__ == "__main__":
    main()
