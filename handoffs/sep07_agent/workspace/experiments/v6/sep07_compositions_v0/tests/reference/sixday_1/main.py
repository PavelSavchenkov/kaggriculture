"""six-day backbone (yhay81, Apache-2.0) + our ES-trained market-denial head.

Backbone: yhay81's Six-Day Public-State Fieldbook -- a public-state router over reviewed six-day
plans (branch points at turns 144/288/432/576, 8 route tapes). Measured on fresh seeds it beats
tfuku-01 +$26.9k and champion-v58 +$24.6k, where our three-day-based agent gets +$15.1k and +$3.7k.

Head: our own 13-parameter additive market-denial policy (see agents/shoprouter-rl). It appends
extra non-wheat SELLs into leftover market slots, never removing a backbone order. On 182 real
ladder games it is worth ~56 wins on the three-day backbone.

OPEN RISK this package exists to measure: the six-day backbone is market-COUPLED (it opens by
buying wheat and carries a budget guard keyed to market prices), so our head's dumping may disturb
its own planning the way it disturbs opponents'. That is exactly why this is measured head-to-head
before any submission.
"""
from __future__ import annotations
import importlib.util
import math
import sys
from pathlib import Path

HEAD_ITEMS = ["CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER"]
MARKET_I0 = 10000
_W = [2.001, -0.131, -0.104, -7.566, -0.574]
_BIAS = [-0.775, -1.163, 1.852, 1.362, -1.676, -1.504, 0.19, -0.948]


def _here():
    """Kaggle's source loader does not define __file__; a function's code object still
    carries the real path of the extracted main.py (same trick the backbone itself uses)."""
    try:
        p = Path(_here.__code__.co_filename).resolve().parent
        if (p / "_sixday_main.py").is_file():
            return p
    except Exception:
        pass
    for cand in (Path("/kaggle_simulations/agent"), Path.cwd(),
                 *[Path(x) for x in sys.path if x]):
        try:
            if (cand / "_sixday_main.py").is_file():
                return cand.resolve()
        except OSError:
            continue
    return Path("/kaggle_simulations/agent")


def _load_backbone():
    here = _here()
    spec = importlib.util.spec_from_file_location(f"{__name__}_sixday", here / "_sixday_main.py")
    m = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = m
    spec.loader.exec_module(m)
    return m


_BACKBONE = _load_backbone()


def _get(o, k, dv=None):
    return o.get(k, dv) if isinstance(o, dict) else getattr(o, k, dv)


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
        mk = list((a or {}).get("market", []) or [])
        room = 10 - len(mk)
        if room <= 0:
            return a
        already = {o[1] for o in mk if o and o[0] == "SELL"}
        sells = _head_sells(observation, seat, already)
        if sells:
            a = {"farmer": (a or {}).get("farmer", ["PASS"]),
                 "hands": (a or {}).get("hands", []),
                 "market": (mk + sells[:room])[:10]}
    except Exception:
        return a
    return a
