"""Option-preserving suffix selection from public state."""

from __future__ import annotations

from typing import Any, Callable

from v23.state_encoder import get


def default_suffix_eligible(obs: Any, decision_step: int = 156) -> bool:
    """Whether the GIN suffix still shares the executed actor/market prefix."""

    if int(get(obs, "step", 0) or 0) != int(decision_step):
        return False
    town = get(obs, "town", {}) or {}
    shops = [str(value) for value in (get(town, "unlocked_shops", []) or [])]
    if shops and shops[0] in {"YARN_STORE", "FARMERS_MARKET"}:
        return False
    if len(shops) >= 2 and shops[1] == "YARN_STORE":
        return False
    return True


def build_suffix_router(
    base_policy: Callable,
    alternative_policy: Callable,
    gate: Callable[[Any], bool],
    *,
    decision_step: int = 156,
    eligible: Callable[[Any, int], bool] = default_suffix_eligible,
):
    """Select exactly once after a prefix shared by both continuations."""

    states = {0: {}, 1: {}}
    telemetry = {
        "games": 0,
        "base_games": 0,
        "alternative_games": 0,
        "ineligible_games": 0,
        "queries": 0,
    }

    def policy(obs: Any, configuration: Any = None) -> dict:
        seat = 1 if int(get(obs, "player", 0) or 0) == 1 else 0
        step = int(get(obs, "step", 0) or 0)
        state = states[seat]
        if not state or step == 0 or step < int(state.get("last_step", -1)):
            state = {"last_step": step, "selected": None}
            states[seat] = state
            telemetry["games"] += 1
        state["last_step"] = step

        if state["selected"] is None and step < int(decision_step):
            return base_policy(obs, configuration)
        if state["selected"] is None:
            can_switch = bool(eligible(obs, int(decision_step)))
            telemetry["queries"] += int(can_switch)
            if can_switch and bool(gate(obs)):
                state["selected"] = "alternative"
                telemetry["alternative_games"] += 1
            else:
                state["selected"] = "base"
                telemetry["base_games"] += 1
                telemetry["ineligible_games"] += int(not can_switch)

        selected = alternative_policy if state["selected"] == "alternative" else base_policy
        return selected(obs, configuration)

    policy.states = states
    policy.telemetry = telemetry
    policy.base_policy = base_policy
    policy.alternative_policy = alternative_policy
    policy.decision_step = int(decision_step)
    return policy
