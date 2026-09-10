"""Portable Kaggriculture seed agent generated from a training checkpoint."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
from typing import Any

from kaggle_environments.envs.kaggriculture.kaggriculture import market_price as _official_market_price


def _directory() -> Path:
    value = globals().get("__file__")
    candidates = [Path(value).resolve().parent] if isinstance(value, str) else []
    candidates += [Path("/kaggle_simulations/agent"), Path.cwd()]
    for candidate in candidates:
        if (candidate / "weights.npz").is_file():
            return candidate
    raise FileNotFoundError("cannot locate exported weights.npz")


_DIR = _directory()


def _sibling(name: str):
    spec = importlib.util.spec_from_file_location(f"{__name__}_{name}", _DIR / f"{name}.py")
    if spec is None or spec.loader is None:
        raise ImportError(name)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


_runtime = _sibling("portable_runtime")
_codec = _sibling("action_codec")
_MODEL = _runtime.PortableActor.load(_DIR / "weights.npz")
_STATE = [None, None]
_LAST_STEP = [-1, -1]


def _fallback(observation: Any) -> dict[str, Any]:
    try:
        player = int(observation["player"])
        hands = observation["farms"][player]["hands"]
        return {"farmer": ["PASS"], "hands": [["PASS"] for _ in hands], "market": []}
    except Exception:
        return {"farmer": ["PASS"], "hands": [], "market": []}


def agent(observation: Any, configuration: Any = None) -> dict[str, Any]:
    try:
        player = int(observation["player"])
        step = int(observation["day"]) * 24 + int(observation["hour"])
        if step == 0 or step <= _LAST_STEP[player]:
            _STATE[player] = _MODEL.initial_state()
        encoded = _runtime.encode_live_observation(observation)
        output = _MODEL.forward(encoded, _STATE[player])
        _STATE[player] = output["state"]
        _LAST_STEP[player] = step
        output["market_step"] = _MODEL.market_step
        market = observation.get("market", {}) if isinstance(observation, dict) else observation.market
        params = market.get("params") if isinstance(market, dict) else market.params

        def price(item: int, inventory: int) -> int:
            return int(_official_market_price(_runtime.ITEM_NAMES[item], inventory, params))

        if isinstance(configuration, dict):
            shed_capacity = int(configuration.get(
                "shedCapacity", configuration.get("shed_capacity", 100)))
        else:
            shed_capacity = int(getattr(
                configuration, "shedCapacity", getattr(configuration, "shed_capacity", 100)))
        return _codec.decode_action(output, encoded, price, shed_capacity)
    except Exception as error:
        # Local diagnostics can opt into fail-fast behavior; submitted agents
        # retain the safe fallback required by the competition runtime.
        if globals().get("_FAIL_FAST", False):
            raise RuntimeError("RL seed inference failed") from error
        return _fallback(observation)


__all__ = ["agent"]
