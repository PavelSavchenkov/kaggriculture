"""six-day backbone + cash-solvency guard + our ES-trained market-denial head.

THE BUG THIS FIXES (diagnosed from 5 busted real ladder games, then reproduced 45/48 on fresh
seeds): hands are a DAILY expense -- fast_sim/sim.hpp `end_of_day` sets n_units = 1 -- and a hand
costs fib(hires_today), i.e. about $1. six-day's opening buys ~53 wheat and funds five HIREs by
reselling it, spending to EXACTLY $0. BUY_PRODUCT is quoted against LIVE market inventory, so when
the opponent buys wheat the same turn the price rises, the resale under-delivers, and the agent
lands on $0 -- where it can no longer afford even one $1 hand. Result: 1 unit, no production, no
income, $0 for 700+ steps. Measured 80/80 against a wheat-trading opponent, 0/80 vs tfuku-01 and
champion-v58.

THE FIX: keep a small cash reserve by trimming (never inventing) purchases.
  reserve 150  ->  busts 45/48 -> 0/48, and the matchups six-day wins stay 48/48.
Two things measured the hard way and worth preserving:
  * declining the contested wheat trade is CATASTROPHIC (busts in every matchup) -- that round-trip
    is the engine of the opening, not an optional risk;
  * the reserve is knife-edged: 150 is safe, 300 hands away a matchup six-day wins 16/16.
"""
from __future__ import annotations
import importlib.util
import math
import sys
from pathlib import Path

RESERVE = 150.0
HEAD_ITEMS = ["CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER"]
MARKET_I0 = 10000
PRICE_FLOOR_SKIP = 3          # a sale at/near the $1 floor neither earns nor denies supply
_W = [2.001, -0.131, -0.104, -7.566, -0.574]
_BIAS = [-0.775, -1.163, 1.852, 1.362, -1.676, -1.504, 0.19, -0.948]
_INNER = "_sixday_main.py"

_MARKET = {"CARROT": (36, 10000), "TOMATO": (60, 10000), "STRAWBERRY": (120, 10000),
           "MELON": (250, 10000), "EGG": (50, 10000), "MILK": (160, 10000),
           "WOOL": (200, 10000), "FERTILIZER": (100, 10000), "WHEAT": (25, 10000)}


def _here():
    try:
        p = Path(_here.__code__.co_filename).resolve().parent
        if (p / _INNER).is_file():
            return p
    except Exception:
        pass
    for cand in (Path("/kaggle_simulations/agent"), Path.cwd(), *[Path(x) for x in sys.path if x]):
        try:
            if (cand / _INNER).is_file():
                return cand.resolve()
        except OSError:
            continue
    return Path("/kaggle_simulations/agent")


def _load_backbone():
    spec = importlib.util.spec_from_file_location(f"{__name__}_sixday", _here() / _INNER)
    m = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = m
    spec.loader.exec_module(m)
    return m


_BACKBONE = _load_backbone()


def _get(o, k, dv=None):
    return o.get(k, dv) if isinstance(o, dict) else getattr(o, k, dv)


def _price(obs, item, inv_delta=0):
    """Observed market price; falls back to the published base when prices are absent."""
    mkt = _get(obs, "market", {}) or {}
    prices = _get(mkt, "prices", {}) or {}
    p = prices.get(item)
    if p:
        return float(p)
    return float(_MARKET.get(item, (25, MARKET_I0))[0])


def _head_sells(obs, seat, already):
    priv = _get(obs, "private", {}) or {}
    shed = _get(priv, "shed", {}) or {}
    minv = _get(_get(obs, "market", {}) or {}, "inventory", {}) or {}
    farms = _get(obs, "farms", []) or [{}, {}]
    step = int(_get(obs, "step", 0) or 0)
    my = float(_get(farms[seat], "money", 0) or 0)
    opp = float(_get(farms[1 - seat], "money", 0) or 0)
    t = step / 720.0
    margin = max(-2.0, min(2.0, (my - opp) / 50000.0))
    out = []
    for i, it in enumerate(HEAD_ITEMS):
        if it in already:
            continue
        q = int(shed.get(it, 0) or 0)
        if q <= 0:
            continue
        if _price(obs, it) < PRICE_FLOOR_SKIP:
            continue
        inv = int(minv.get(it, MARKET_I0) or MARKET_I0)
        feats = [min(q, 100) / 50.0, (inv - MARKET_I0) / 10000.0, t, margin, 1.0]
        logit = _BIAS[i] + sum(w * f for w, f in zip(_W, feats))
        frac = 1.0 / (1.0 + math.exp(-max(-30.0, min(30.0, logit))))
        n = int(round(frac * q))
        if n > 0:
            out.append(["SELL", it, n])
    return out


def agent(observation, configuration=None):
    a = _BACKBONE.agent(observation, configuration)
    try:
        seat = int(_get(observation, "player", 0) or 0)
        farms = _get(observation, "farms", []) or [{}]
        cash = float(_get(farms[seat], "money", 0) or 0)
        orders = [list(o) for o in ((a or {}).get("market", []) or []) if o]
        kept = []
        for o in orders:
            if o[0] == "BUY_PRODUCT" and len(o) > 2:
                px = max(1.0, _price(observation, o[1]))
                n = min(int(o[2]), int(max(0.0, cash - RESERVE) // px))
                if n > 0:
                    kept.append([o[0], o[1], n]); cash -= n * px
                continue                       # trim, never invent
            kept.append(o)
        already = {o[1] for o in kept if o and o[0] == "SELL" and len(o) > 1}
        room = 10 - len(kept)
        if room > 0:
            kept += _head_sells(observation, seat, already)[:room]
        return {"farmer": (a or {}).get("farmer", ["PASS"]),
                "hands": (a or {}).get("hands", []), "market": kept[:10]}
    except Exception:
        return a
