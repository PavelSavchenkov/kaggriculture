"""Build a closed-loop controller over one independently screened route prior."""

from __future__ import annotations

import copy
from typing import Any

from v23.planner import PlannerConfig, build_sparse_planner
from v49.residual_controller import ResidualConfig, build_residual_controller


def build_route_value_hybrid(
    actions: list[dict],
    config: ResidualConfig = ResidualConfig(),
):
    """Use a complete route only as a legal proposal for residual decisions.

    The sparse planner repairs observable weed/hand drift.  Market timing and
    quantity remain observation-conditioned in ``ResidualValueController``.
    """

    route = copy.deepcopy(actions)
    if len(route) != 719:
        raise ValueError("v50 route prior must contain exactly 719 actions")
    base = build_sparse_planner(route, PlannerConfig())
    policy = build_residual_controller(
        base,
        {"default": route},
        route_selector=None,
        config=config,
    )
    policy.route_prior = route
    policy.architecture = {
        "route_role": "proposal prior",
        "observable_drift_repair": True,
        "public_market_flow": True,
        "runtime_identity_features": False,
        "runtime_lineage_features": False,
        "future_opponent_actions": False,
    }
    return policy


def safe_action(obs: Any) -> dict:
    seat = 1 if int((obs or {}).get("player", 0) or 0) == 1 else 0
    farms = list((obs or {}).get("farms", []) or [])
    farm = farms[seat] if seat < len(farms) else {}
    return {
        "farmer": ["PASS"],
        "hands": [["PASS"] for _ in (farm.get("hands", []) or [])],
        "market": [],
    }
