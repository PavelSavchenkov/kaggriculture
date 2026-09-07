"""shoprouter-rl -- strong public production backbone + an ORIGINAL RL-trained market-denial head.

Backbone: yhay81's "Three-Day Shop Router" (Apache-2.0), loaded from _shop_bridge.py (its ctypes
bridge to agent.so). We keep its ENTIRE action intact -- farmer, hands, all buys, all its sells --
so its cash flow / production stays calibrated (grafting-replaces its selling breaks it).

Head (ours): an additive market-denial policy that appends extra SELLs of NON-WHEAT products in
leftover market slots. It captures idle-market drains and pre-empts the opponent. Parameters were
trained by Evolution Strategies against a MIXED opponent panel (shoprouter/tfuku/starter),
optimizing the terminal-margin DELTA over raw shoprouter so it gains vs shoprouter-clones without
regressing vs the rest of the field.

Measured (fast_sim) vs raw shoprouter: +$3.2k head-to-head (8/8, two seed blocks); vs tfuku/top10/
starter: within +-$300 of raw shoprouter (noise) -- a near-strict improvement over our own entry.

Pure-Python head (no numpy) so it runs anywhere the backbone does.
"""
from __future__ import annotations
import importlib.util
import math
import sys
from pathlib import Path


def _resolve_dir() -> Path:
    fv = globals().get("__file__")
    if isinstance(fv, str) and fv:
        return Path(fv).resolve().parent
    for cand in [Path("/kaggle_simulations/agent"), Path.cwd(), *[Path(p) for p in sys.path if p]]:
        try:
            if (cand / "_shop_bridge.py").is_file():
                return cand.resolve()
        except OSError:
            continue
    return Path("/kaggle_simulations/agent")


def _load_bridge():
    mod = f"{__name__}_shop_bridge"
    if mod in sys.modules:
        return sys.modules[mod]
    spec = importlib.util.spec_from_file_location(mod, _resolve_dir() / "_shop_bridge.py")
    m = importlib.util.module_from_spec(spec)
    sys.modules[mod] = m
    spec.loader.exec_module(m)
    return m


_BRIDGE = _load_bridge()   # exposes .agent(obs) -> {farmer, hands, market}

# --- learned market-denial head (ES-trained, mixed-panel) ---------------------------------
_HEAD_ITEMS = ["CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER"]
# theta = shared feature weights w[5] + per-product bias[8]; features = [shed_frac, inv_fullness,
# time, margin, 1.0]  (see rl_markethead/env.py). Frozen from theta_panel.npy (heldout mean-delta +$877).
_W = [2.001, -0.131, -0.104, -7.566, -0.574]
_BIAS = [-0.775, -1.163, 1.852, 1.362, -1.676, -1.504, 0.19, -0.948]
_MARKET_I0 = 10000


def _get(o, k, dv=None):
    if isinstance(o, dict):
        return o.get(k, dv)
    return getattr(o, k, dv)


def _head_sells(obs, seat, already):
    priv = _get(obs, "private", {}) or {}
    shed = _get(priv, "shed", {}) or {}
    market = _get(obs, "market", {}) or {}
    minv = _get(market, "inventory", {}) or {}
    prices = _get(market, "prices", {}) or {}
    farms = _get(obs, "farms", []) or [{}, {}]
    step = int(_get(obs, "step", 0) or 0)
    my = float(_get(farms[seat], "money", 0) or 0)
    opp = float(_get(farms[1 - seat], "money", 0) or 0)
    t = step / 720.0
    margin = max(-2.0, min(2.0, (my - opp) / 50000.0))
    out = []
    for i, it in enumerate(_HEAD_ITEMS):
        if it in already:
            continue
        q = int(shed.get(it, 0) or 0)
        if q <= 0:
            continue
        # PRICE-FLOOR SKIP (measured +1 game over 182 real ladder games, 131/182 vs 130/182):
        # a sale at the $1 floor yields $1 AND does not add supply (`if (price > 1) inventory += 1`
        # in the engine), so it neither earns nor denies -- the unit is strictly wasted, and the
        # town drain will re-price it if held.
        _px = (prices.get(it) if isinstance(prices, dict) else None)
        if _px is not None and float(_px) < 3.0:
            continue
        inv = int(minv.get(it, _MARKET_I0) or _MARKET_I0)
        feats = [min(q, 100) / 50.0, (inv - _MARKET_I0) / 10000.0, t, margin, 1.0]
        logit = _BIAS[i] + sum(w * f for w, f in zip(_W, feats))
        frac = 1.0 / (1.0 + math.exp(-max(-30.0, min(30.0, logit))))
        qty = int(round(frac * q))
        if qty > 0:
            out.append(["SELL", it, qty])
    return out


def agent(obs, configuration=None):
    a = _BRIDGE.agent(obs, configuration)
    try:
        seat = int(_get(obs, "player", 0) or 0)
        mk = list(a.get("market", []) or [])
        already = {o[1] for o in mk if o and o[0] == "SELL"}
        room = 10 - len(mk)
        if room > 0:
            sells = _head_sells(obs, seat, already)
            if sells:
                a = {"farmer": a.get("farmer", ["PASS"]), "hands": a.get("hands", []),
                     "market": (mk + sells[:room])[:10]}
    except Exception:
        return a
    return a


__all__ = ["agent"]
