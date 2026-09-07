"""Observation-conditioned residual controller over a validated proposal policy.

The route library is a proposal generator, not the deployed decision rule.  On
every turn this controller estimates the opponent's recent market flow from
observable inventory changes, values legal SELL timing alternatives, protects
feed and investment capital, and executes only a bounded positive residual.

No team identity, submission id, action hash, opponent private inventory, or
future opponent action is available at runtime.
"""

from __future__ import annotations

import copy
from dataclasses import dataclass
import math
from typing import Any, Callable

from scripts.v19_terminal import align_hands, clone_distance, terminal_market
from v23.policy_library import reorder_sell_slots
from v23.simulator import execute_sell, market_price
from v23.state_encoder import BASE_PRICE, SHOP_PRODUCTS, get
from v24.market_maker import (
    MarketMakerConfig,
    MarketMakerExpert,
    _animal_count,
    _project_private_after_actor,
)


PRODUCTS = tuple(BASE_PRICE)


@dataclass(frozen=True)
class ResidualConfig:
    active_start: int = 72
    active_stop: int = 700
    maximum_orders: int = 10
    # Public-state opponent-flow inference.
    flow_alpha: float = 0.35
    maximum_inferred_supply: int = 80
    near_distance: float = 2.0
    near_streak_required: int = 18
    # Existing queue control.
    front_existing_sells: bool = True
    front_on_near: bool = True
    front_money_deficit: float = 4500.0
    front_supply_threshold: float = 3.0
    # WHEAT/FERTILIZER are operational inputs.  Moving their sales forward can
    # subsidise an opponent's feed/investment chain and create a large cascade.
    front_items: tuple[str, ...] = (
        "CARROT",
        "TOMATO",
        "STRAWBERRY",
        "MELON",
        "EGG",
        "MILK",
        "WOOL",
    )
    # Residual action-value SELL timing.
    preempt_enabled: bool = True
    preempt_horizon: int = 3
    preempt_maximum_batch: int = 12
    preempt_minimum_delta: float = 8.0
    preempt_minimum_price_ratio: float = 0.45
    preempt_exposure_scale: float = 0.08
    preempt_on_near: bool = True
    # A value of 1 reproduces the old copy assumption that a near opponent
    # will execute our scheduled future SELL.  Zero makes the forecast rely on
    # observed market flow and visible production exposure only.  This is the
    # principal replay-specific-vs-generalized ablation.
    near_scheduled_supply_weight: float = 1.0
    adaptive_near_schedule_weight: bool = False
    adaptive_confidence_alpha: float = 0.35
    adaptive_confidence_negative_alpha: float = 0.35
    adaptive_confidence_decay: float = 0.995
    adaptive_confidence_minimum_events: int = 1
    adaptive_confidence_activation_threshold: float = 0.0
    adaptive_confidence_ceiling: float = 1.0
    preempt_money_deficit: float = 4000.0
    feed_days_reserve: float = 1.5
    preempt_items: tuple[str, ...] = (
        "CARROT",
        "TOMATO",
        "STRAWBERRY",
        "MELON",
        "EGG",
        "MILK",
        "WOOL",
    )
    # One-tick demand recovery branch.  Disabled unless selected by validation.
    defer_enabled: bool = False
    defer_start: int = 240
    defer_stop: int = 680
    defer_maximum_batch: int = 12
    defer_maximum_hold: int = 2
    defer_minimum_gain: float = 20.0
    defer_minimum_cash: float = 3500.0
    defer_items: tuple[str, ...] = (
        "CARROT",
        "STRAWBERRY",
        "MELON",
        "MILK",
        "WOOL",
    )
    # Independent, reserve-safe market-maker expert.
    market_maker_enabled: bool = False
    market_maker_start: int = 300
    market_maker_batch: int = 8
    market_maker_minimum_profit: float = 20.0
    market_maker_cash_reserve: float = 3000.0
    # Last-turn inventory is always converted to money.
    terminal_rule: str = "collision"


def _seat(obs: Any) -> int:
    return 1 if int(get(obs, "player", 0) or 0) == 1 else 0


def _safe(action: Any, obs: Any) -> dict:
    copied = copy.deepcopy(action) if isinstance(action, dict) else {}
    copied["farmer"] = list(copied.get("farmer") or ["PASS"])
    copied["hands"] = [
        list(order or ["PASS"]) for order in (copied.get("hands") or [])
    ]
    copied["market"] = [
        list(order)
        for order in (copied.get("market") or [])
        if isinstance(order, (list, tuple)) and order
    ]
    return align_hands(copied, obs)


def _market_net(rows: list[list]) -> dict[str, int]:
    result = {item: 0 for item in PRODUCTS}
    for order in rows:
        if len(order) < 3:
            continue
        operation, item = str(order[0]), str(order[1])
        if item not in result or operation not in {"SELL", "BUY_PRODUCT"}:
            continue
        try:
            quantity = max(0, int(order[2]))
        except (TypeError, ValueError):
            continue
        result[item] += quantity if operation == "SELL" else -quantity
    return result


def _executable_market_net(
    obs: Any, action: dict, configuration: Any
) -> dict[str, int]:
    """Conservative own market flow, clipped to observable legal inventory.

    Fixed schedules often request a quantity larger than the shed and rely on
    the engine stopping the order.  Treating the requested quantity as an
    executed sale would systematically erase inferred opponent supply.
    """

    capacity = max(1, int(get(configuration, "shedCapacity", 100) or 100))
    shed, _inventories = _project_private_after_actor(
        obs, action, shed_capacity=capacity
    )
    result = {item: 0 for item in PRODUCTS}
    for raw in action.get("market", []) or []:
        order = list(raw)
        if len(order) < 3:
            continue
        operation, item = str(order[0]), str(order[1])
        if item not in result or operation not in {"SELL", "BUY_PRODUCT"}:
            continue
        try:
            requested = max(0, int(order[2]))
        except (TypeError, ValueError):
            continue
        if operation == "SELL":
            executed = min(requested, max(0, int(shed.get(item, 0) or 0)))
            shed[item] = max(0, int(shed.get(item, 0) or 0) - executed)
            result[item] += executed
        elif item in {"WHEAT", "FERTILIZER"}:
            room = max(0, capacity - sum(max(0, int(value or 0)) for value in shed.values()))
            executed = min(requested, room)
            shed[item] = int(shed.get(item, 0) or 0) + executed
            result[item] -= executed
    return result


def _sell_totals(rows: list[list]) -> dict[str, int]:
    result: dict[str, int] = {}
    for order in rows:
        if len(order) < 3 or str(order[0]) != "SELL":
            continue
        try:
            quantity = max(0, int(order[2]))
        except (TypeError, ValueError):
            continue
        item = str(order[1])
        if item in PRODUCTS:
            result[item] = result.get(item, 0) + quantity
    return result


def _demand_at(shops: list[str], step: int, configuration: Any, item: str) -> int:
    shop_interval = max(1, int(get(configuration, "townShopSellInterval", 4) or 4))
    center_interval = max(1, int(get(configuration, "townCenterSellInterval", 24) or 24))
    demand = 0
    if int(step) % shop_interval == 0:
        for shop in shops:
            offered = SHOP_PRODUCTS.get(str(shop), ())
            if item in offered:
                demand += 2 if len(offered) == 1 else 1
    if item != "FERTILIZER" and int(step) % center_interval == 0:
        demand += 1
    return demand


def _future_demand(
    shops: list[str], step: int, horizon: int, configuration: Any, item: str
) -> int:
    return sum(
        _demand_at(shops, future, configuration, item)
        for future in range(int(step), int(step) + max(0, int(horizon)))
    )


def _public_exposure(farm: dict) -> dict[str, float]:
    result = {item: 0.0 for item in PRODUCTS}
    animal_product = {"COW": "MILK", "SHEEP": "WOOL", "GOOSE": "EGG"}
    for row in get(farm, "tiles", []) or []:
        for tile in row if isinstance(row, list) else [row]:
            if not isinstance(tile, dict):
                continue
            crop = str(tile.get("crop", "") or "")
            animal = str(tile.get("animal", "") or "")
            yield_units = max(1, int(tile.get("yield_units", 0) or 0))
            if crop in result:
                result[crop] += yield_units
            product = animal_product.get(animal)
            if product is not None:
                result[product] += yield_units
            if bool(tile.get("fertilizer_available", False)):
                result["FERTILIZER"] += 1.0
    return result


def _money_gap(obs: Any) -> float:
    seat = _seat(obs)
    farms = list(get(obs, "farms", []) or [])
    if len(farms) != 2:
        return 0.0
    own = farms[seat] if isinstance(farms[seat], dict) else {}
    opponent = farms[1 - seat] if isinstance(farms[1 - seat], dict) else {}
    return float(get(own, "money", 0) or 0) - float(get(opponent, "money", 0) or 0)


def _revenue(item: str, inventory: int, quantity: int) -> float:
    value, _ending = execute_sell(item, int(inventory), max(0, int(quantity)))
    return float(value)


def _future_allocations(
    route: list[dict],
    *,
    step: int,
    horizon: int,
    item: str,
    maximum: int,
    due: dict[int, dict[str, int]],
) -> list[tuple[int, int]]:
    remaining = max(0, int(maximum))
    result = []
    stop = min(len(route) - 1, int(step) + max(1, int(horizon)))
    for future in range(int(step) + 1, stop + 1):
        planned = _sell_totals(
            [list(order) for order in ((route[future] or {}).get("market", []) or [])]
        ).get(item, 0)
        planned = max(0, planned - int((due.get(future, {}) or {}).get(item, 0) or 0))
        quantity = min(remaining, planned)
        if quantity:
            result.append((future, quantity))
            remaining -= quantity
        if remaining <= 0:
            break
    return result


def _sort_sells_front(
    obs: Any,
    action: dict,
    configuration: Any,
    items: tuple[str, ...],
) -> dict:
    ordered = reorder_sell_slots(obs, action, configuration, demand_alpha=0.25)
    market = [list(order) for order in (ordered.get("market", []) or [])]
    allowed = {str(item) for item in items}
    sells = [
        order
        for order in market
        if order
        and str(order[0]) == "SELL"
        and len(order) >= 2
        and str(order[1]) in allowed
    ]
    others = [order for order in market if order not in sells]
    ordered["market"] = sells + others
    return ordered


class ResidualValueController:
    """Choose bounded action residuals from current observable value."""

    def __init__(
        self,
        base_policy: Callable,
        routes: dict[str, list[dict]],
        route_selector: Callable[[Any], str] | None = None,
        config: ResidualConfig = ResidualConfig(),
    ) -> None:
        if not routes or any(len(actions) != 719 for actions in routes.values()):
            raise ValueError("residual controller requires complete 719-turn routes")
        self.base_policy = base_policy
        self.routes = copy.deepcopy(routes)
        self.route_selector = route_selector
        self.config = config
        self.states = {0: {}, 1: {}}
        self.market_makers = {
            name: MarketMakerExpert(
                actions,
                MarketMakerConfig(
                    enabled=bool(config.market_maker_enabled),
                    item="WHEAT",
                    feed_item="WHEAT",
                    start_step=int(config.market_maker_start),
                    stop_entry_step=716,
                    max_batch=max(0, int(config.market_maker_batch)),
                    minimum_expected_profit=float(config.market_maker_minimum_profit),
                    mirror_minimum_expected_profit=float(config.market_maker_minimum_profit),
                    minimum_cash_reserve=float(config.market_maker_cash_reserve),
                    feed_days_reserve=float(config.feed_days_reserve),
                    investment_horizon=2,
                    price_cost_buffer=1.05,
                    shed_headroom=15,
                    max_orders=int(config.maximum_orders),
                ),
            )
            for name, actions in self.routes.items()
        }
        self.telemetry = {
            "games": 0,
            "calls": 0,
            "near_turns": 0,
            "near_latches": 0,
            "flow_updates": 0,
            "mirror_evidence_turns": 0,
            "mirror_confidence_sum": 0.0,
            "mirror_confidence_peak": 0.0,
            "front_turns": 0,
            "preempt_turns": 0,
            "preempt_units": 0,
            "preempt_value": 0.0,
            "repaid_units": 0,
            "defer_turns": 0,
            "defer_units": 0,
            "release_turns": 0,
            "release_units": 0,
            "routes": {name: 0 for name in self.routes},
        }

    def _reset(self, seat: int, step: int) -> dict:
        state = {
            "last_step": step,
            "near_streak": 0,
            "near_latched": False,
            "last_inventory": None,
            "last_market_net": {item: 0 for item in PRODUCTS},
            "last_shops": [],
            "opponent_supply": {item: 0.0 for item in PRODUCTS},
            "mirror_confidence": 0.0,
            "mirror_evidence_turns": 0,
            "due": {},
            "pending": {},
            "route": "default" if "default" in self.routes else next(iter(self.routes)),
        }
        self.states[seat] = state
        self.telemetry["games"] += 1
        return state

    def _route_name(self, obs: Any, state: dict) -> str:
        if self.route_selector is None:
            return str(state.get("route"))
        selected = str(self.route_selector(obs))
        return selected if selected in self.routes else str(state.get("route"))

    def _update_flow(self, obs: Any, configuration: Any, state: dict, step: int) -> None:
        market = get(obs, "market", {}) or {}
        current = {
            item: int(get(get(market, "inventory", {}) or {}, item, 10000) or 10000)
            for item in PRODUCTS
        }
        previous = state.get("last_inventory")
        if isinstance(previous, dict) and int(state.get("last_step", step)) == step - 1:
            alpha = min(1.0, max(0.0, float(self.config.flow_alpha)))
            own_net = dict(state.get("last_market_net", {}) or {})
            shops = list(state.get("last_shops", []) or [])
            supply = state.setdefault("opponent_supply", {})
            inferred_rows = {}
            for item in PRODUCTS:
                inferred = (
                    current[item]
                    - int(previous.get(item, current[item]) or current[item])
                    - int(own_net.get(item, 0) or 0)
                    + _demand_at(shops, step - 1, configuration, item)
                )
                inferred = min(
                    max(0, inferred), max(0, int(self.config.maximum_inferred_supply))
                )
                inferred_rows[item] = inferred
                supply[item] = (1.0 - alpha) * float(supply.get(item, 0.0)) + alpha * inferred
            selected = {str(item) for item in self.config.preempt_items}
            own_total = sum(
                max(0, int(own_net.get(item, 0) or 0)) for item in selected
            )
            opponent_total = sum(
                max(0, int(inferred_rows.get(item, 0) or 0)) for item in selected
            )
            if own_total > 0 or opponent_total > 0:
                overlap = sum(
                    min(
                        max(0, int(own_net.get(item, 0) or 0)),
                        max(0, int(inferred_rows.get(item, 0) or 0)),
                    )
                    for item in selected
                )
                agreement = (
                    2.0 * overlap / max(1.0, float(own_total + opponent_total))
                )
                previous_confidence = float(state.get("mirror_confidence", 0.0))
                alpha_name = (
                    "adaptive_confidence_negative_alpha"
                    if agreement < previous_confidence
                    else "adaptive_confidence_alpha"
                )
                belief_alpha = min(
                    1.0,
                    max(0.0, float(getattr(self.config, alpha_name))),
                )
                confidence = (
                    (1.0 - belief_alpha)
                    * previous_confidence
                    + belief_alpha * agreement
                )
                confidence = min(
                    max(0.0, float(self.config.adaptive_confidence_ceiling)),
                    max(0.0, confidence),
                )
                state["mirror_confidence"] = confidence
                state["mirror_evidence_turns"] = int(
                    state.get("mirror_evidence_turns", 0)
                ) + 1
                self.telemetry["mirror_evidence_turns"] += 1
                self.telemetry["mirror_confidence_sum"] += confidence
                self.telemetry["mirror_confidence_peak"] = max(
                    float(self.telemetry["mirror_confidence_peak"]), confidence
                )
            else:
                state["mirror_confidence"] = (
                    float(state.get("mirror_confidence", 0.0))
                    * min(
                        1.0,
                        max(0.0, float(self.config.adaptive_confidence_decay)),
                    )
                )
            self.telemetry["flow_updates"] += 1
        state["last_inventory"] = current

    def _repay(self, action: dict, state: dict, step: int) -> dict:
        due_now = dict(state.setdefault("due", {}).pop(step, {}) or {})
        if not due_now:
            return action
        adjusted = []
        repaid = 0
        for raw in action.get("market", []) or []:
            order = list(raw)
            if len(order) >= 3 and str(order[0]) == "SELL":
                item = str(order[1])
                debt = max(0, int(due_now.get(item, 0) or 0))
                if debt:
                    quantity = max(0, int(order[2] or 0))
                    reduction = min(quantity, debt)
                    quantity -= reduction
                    due_now[item] = debt - reduction
                    repaid += reduction
                    if quantity <= 0:
                        continue
                    order[2] = quantity
            adjusted.append(order)
        for item, quantity in due_now.items():
            if quantity > 0 and step < 718:
                carry = state.setdefault("due", {}).setdefault(step + 1, {})
                carry[item] = int(carry.get(item, 0) or 0) + quantity
        action["market"] = adjusted
        self.telemetry["repaid_units"] += repaid
        return action

    def _release_pending(self, obs: Any, action: dict, state: dict, step: int) -> dict:
        pending = state.setdefault("pending", {})
        if not pending or len(action["market"]) >= int(self.config.maximum_orders):
            return action
        capacity = int(get(None, "shedCapacity", 100) or 100)
        projected, _inventories = _project_private_after_actor(
            obs, action, shed_capacity=capacity
        )
        existing = _sell_totals(action["market"])
        released = 0
        rows = []
        for item, record in list(pending.items()):
            due = int(record.get("release_step", step) or step)
            age = step - int(record.get("entry_step", step) or step)
            if step < due and age < int(self.config.defer_maximum_hold):
                continue
            owed = max(0, int(record.get("quantity", 0) or 0))
            quantity = min(
                owed,
                max(0, int(projected.get(item, 0) or 0) - int(existing.get(item, 0))),
            )
            if quantity > 0:
                price = float(get(get(obs, "market", {}) or {}, "prices", {}).get(item, 0) or 0)
                rows.append((price * quantity, item, quantity))
        for _value, item, quantity in sorted(rows, reverse=True):
            if len(action["market"]) >= int(self.config.maximum_orders):
                break
            action["market"].insert(0, ["SELL", item, quantity])
            pending[item]["quantity"] = max(0, int(pending[item]["quantity"]) - quantity)
            if pending[item]["quantity"] <= 0:
                pending.pop(item, None)
            released += quantity
        if released:
            self.telemetry["release_turns"] += 1
            self.telemetry["release_units"] += released
        return action

    def _defer(self, obs: Any, action: dict, state: dict, configuration: Any, step: int) -> dict:
        if not (
            bool(self.config.defer_enabled)
            and int(self.config.defer_start) <= step < int(self.config.defer_stop)
            and len(state.get("pending", {})) < 3
        ):
            return action
        if any(order and str(order[0]) != "SELL" for order in action["market"]):
            return action
        farms = list(get(obs, "farms", []) or [])
        own = farms[_seat(obs)] if farms else {}
        if float(get(own, "money", 0) or 0) < float(self.config.defer_minimum_cash):
            return action
        shops = list(get(get(obs, "town", {}) or {}, "unlocked_shops", []) or [])
        market = get(obs, "market", {}) or {}
        inventories = get(market, "inventory", {}) or {}
        allowed = set(self.config.defer_items)
        remaining = max(0, int(self.config.defer_maximum_batch))
        kept = []
        deferred = 0
        for raw in action["market"]:
            order = list(raw)
            if len(order) < 3 or str(order[0]) != "SELL" or str(order[1]) not in allowed:
                kept.append(order)
                continue
            item = str(order[1])
            demand = _demand_at(shops, step, configuration, item)
            if demand <= 0 or remaining <= 0:
                kept.append(order)
                continue
            quantity = min(max(0, int(order[2] or 0)), remaining)
            inventory = int(get(inventories, item, 10000) or 10000)
            now = _revenue(item, inventory, quantity)
            later = _revenue(item, inventory - demand, quantity)
            # Recent opponent flow is charged to the delayed quote.
            supply = int(math.ceil(float(state.get("opponent_supply", {}).get(item, 0.0))))
            later = _revenue(item, inventory - demand + supply, quantity)
            if later - now < float(self.config.defer_minimum_gain):
                kept.append(order)
                continue
            original = max(0, int(order[2] or 0))
            left = original - quantity
            if left:
                order[2] = left
                kept.append(order)
            record = state.setdefault("pending", {}).setdefault(
                item,
                {"quantity": 0, "entry_step": step, "release_step": step + 1},
            )
            record["quantity"] = int(record.get("quantity", 0) or 0) + quantity
            record["release_step"] = min(int(record.get("release_step", step + 1)), step + 1)
            remaining -= quantity
            deferred += quantity
        action["market"] = kept
        if deferred:
            self.telemetry["defer_turns"] += 1
            self.telemetry["defer_units"] += deferred
        return action

    def _preempt(
        self,
        obs: Any,
        action: dict,
        route: list[dict],
        state: dict,
        configuration: Any,
        step: int,
    ) -> dict:
        if not (
            bool(self.config.preempt_enabled)
            and int(self.config.active_start) <= step < int(self.config.active_stop)
            and len(action["market"]) < int(self.config.maximum_orders)
        ):
            return action
        seat = _seat(obs)
        farms = list(get(obs, "farms", []) or [])
        own_farm = farms[seat] if seat < len(farms) else {}
        opponent = farms[1 - seat] if len(farms) == 2 else {}
        exposure = _public_exposure(opponent)
        near = bool(state.get("near_latched", False))
        deficit = -_money_gap(obs)
        supply = dict(state.get("opponent_supply", {}) or {})
        if not (
            (near and bool(self.config.preempt_on_near))
            or deficit >= float(self.config.preempt_money_deficit)
            or max(supply.values(), default=0.0) >= float(self.config.front_supply_threshold)
        ):
            return action
        capacity = max(1, int(get(configuration, "shedCapacity", 100) or 100))
        projected, projected_inventories = _project_private_after_actor(
            obs, action, shed_capacity=capacity
        )
        existing = _sell_totals(action["market"])
        available = {
            item: max(0, int(projected.get(item, 0) or 0) - int(existing.get(item, 0)))
            for item in PRODUCTS
        }
        required_wheat = int(
            math.ceil(_animal_count(own_farm) * max(0.0, float(self.config.feed_days_reserve)))
        )
        total_wheat = int(projected.get("WHEAT", 0) or 0) + sum(
            int(inventory.get("WHEAT", 0) or 0) for inventory in projected_inventories
        )
        available["WHEAT"] = min(
            available.get("WHEAT", 0), max(0, total_wheat - required_wheat)
        )
        market = get(obs, "market", {}) or {}
        inventories = get(market, "inventory", {}) or {}
        prices = get(market, "prices", {}) or {}
        shops = list(get(get(obs, "town", {}) or {}, "unlocked_shops", []) or [])
        horizon = max(1, int(self.config.preempt_horizon))
        candidates = []
        selected_items = {str(item) for item in self.config.preempt_items}
        for item in PRODUCTS:
            if item not in selected_items:
                continue
            if available.get(item, 0) <= 0:
                continue
            allocations = _future_allocations(
                route,
                step=step,
                horizon=horizon,
                item=item,
                maximum=min(available[item], int(self.config.preempt_maximum_batch)),
                due=state.setdefault("due", {}),
            )
            scheduled = sum(quantity for _future, quantity in allocations)
            if scheduled <= 0:
                continue
            current_price = float(get(prices, item, BASE_PRICE[item]) or BASE_PRICE[item])
            if current_price / max(1.0, float(BASE_PRICE[item])) < float(
                self.config.preempt_minimum_price_ratio
            ):
                continue
            inferred = float(supply.get(item, 0.0))
            forecast_supply = horizon * (
                inferred + float(self.config.preempt_exposure_scale) * float(exposure.get(item, 0.0))
            )
            if near:
                scheduled_weight = float(
                    self.config.near_scheduled_supply_weight
                )
                if bool(self.config.adaptive_near_schedule_weight):
                    evidence = int(state.get("mirror_evidence_turns", 0))
                    confidence = float(state.get("mirror_confidence", 0.0))
                    enough_evidence = evidence >= max(
                        1,
                        int(self.config.adaptive_confidence_minimum_events),
                    )
                    active_confidence = confidence >= max(
                        0.0,
                        float(
                            self.config.adaptive_confidence_activation_threshold
                        ),
                    )
                    scheduled_weight = (
                        confidence if enough_evidence and active_confidence else 0.0
                    )
                forecast_supply += (
                    max(0.0, scheduled_weight)
                    * scheduled
                )
            demand = _future_demand(shops, step, horizon, configuration, item)
            inventory = int(get(inventories, item, 10000) or 10000)
            future_inventory = int(round(inventory + forecast_supply - demand))
            now = _revenue(item, inventory, scheduled)
            later = _revenue(item, future_inventory, scheduled)
            delta = now - later
            if delta < float(self.config.preempt_minimum_delta):
                continue
            candidates.append((delta, now, item, scheduled, allocations))
        remaining = max(0, int(self.config.preempt_maximum_batch))
        sold = 0
        value = 0.0
        additions = []
        for delta, _gross, item, scheduled, allocations in sorted(candidates, reverse=True):
            if len(action["market"]) + len(additions) >= int(self.config.maximum_orders) or remaining <= 0:
                break
            quantity = min(scheduled, remaining)
            if quantity <= 0:
                continue
            additions.append(["SELL", item, quantity])
            left = quantity
            for due_step, capacity_due in allocations:
                allocated = min(left, capacity_due)
                if allocated:
                    row = state.setdefault("due", {}).setdefault(due_step, {})
                    row[item] = int(row.get(item, 0) or 0) + allocated
                    left -= allocated
                if left <= 0:
                    break
            remaining -= quantity
            sold += quantity
            value += delta * quantity / max(1, scheduled)
        if additions:
            action["market"] = additions + action["market"]
            self.telemetry["preempt_turns"] += 1
            self.telemetry["preempt_units"] += sold
            self.telemetry["preempt_value"] += value
        return action

    def apply(self, obs: Any, configuration: Any = None) -> dict:
        seat = _seat(obs)
        step = int(get(obs, "step", 0) or 0)
        state = self.states[seat]
        if not state or step == 0 or step < int(state.get("last_step", -1)):
            state = self._reset(seat, step)
        # Flow inference must use the action emitted on the previous call.
        self._update_flow(obs, configuration, state, step)

        distance = clone_distance(obs)
        if distance <= float(self.config.near_distance):
            state["near_streak"] = int(state.get("near_streak", 0)) + 1
            self.telemetry["near_turns"] += 1
        else:
            state["near_streak"] = 0
        if (
            not bool(state.get("near_latched", False))
            and int(state.get("near_streak", 0)) >= max(1, int(self.config.near_streak_required))
        ):
            state["near_latched"] = True
            self.telemetry["near_latches"] += 1

        action = _safe(self.base_policy(obs, configuration), obs)
        route_name = self._route_name(obs, state)
        state["route"] = route_name
        route = self.routes[route_name]
        self.telemetry["routes"][route_name] += 1
        action = self._repay(action, state, step)
        action = self._release_pending(obs, action, state, step)
        action = self._defer(obs, action, state, configuration, step)
        action = self._preempt(obs, action, route, state, configuration, step)

        front_items = {str(item) for item in self.config.front_items}
        supply_peak = max(
            (
                float(value)
                for item, value in state.get("opponent_supply", {}).items()
                if item in front_items
            ),
            default=0.0,
        )
        should_front = bool(self.config.front_existing_sells) and (
            (bool(self.config.front_on_near) and bool(state.get("near_latched", False)))
            or -_money_gap(obs) >= float(self.config.front_money_deficit)
            or supply_peak >= float(self.config.front_supply_threshold)
        )
        before = copy.deepcopy(action.get("market", []))
        if should_front:
            action = _sort_sells_front(
                obs, action, configuration, self.config.front_items
            )
        else:
            action = reorder_sell_slots(obs, action, configuration, demand_alpha=0.25)
        if action.get("market", []) != before:
            self.telemetry["front_turns"] += 1

        if bool(self.config.market_maker_enabled):
            action = self.market_makers[route_name].apply(obs, action, configuration)
        if step == 718 and str(self.config.terminal_rule) != "none":
            action = terminal_market(
                obs, action, rule=str(self.config.terminal_rule), replace=True
            )

        action = _safe(action, obs)
        action["market"] = action["market"][: max(1, int(self.config.maximum_orders))]
        state["last_market_net"] = _executable_market_net(
            obs, action, configuration
        )
        state["last_shops"] = list(
            get(get(obs, "town", {}) or {}, "unlocked_shops", []) or []
        )
        state["last_step"] = step
        self.telemetry["calls"] += 1
        return action


def build_residual_controller(
    base_policy: Callable,
    routes: dict[str, list[dict]],
    route_selector: Callable[[Any], str] | None = None,
    config: ResidualConfig = ResidualConfig(),
):
    controller = ResidualValueController(base_policy, routes, route_selector, config)

    def agent(obs: Any, configuration: Any = None) -> dict:
        return controller.apply(obs, configuration)

    agent.controller = controller
    agent.telemetry = controller.telemetry
    agent.residual_config = config
    agent.base_policy = base_policy
    return agent
