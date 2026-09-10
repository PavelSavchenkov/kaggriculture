"""Shared legality masks and structured action decoding.

This module is deliberately NumPy-only so the trainer and exported submission
execute the same autoregressive factorization.  It consumes the canonical
actor observation produced by ``ReplayCursor.observe`` or
``portable_runtime.encode_live_observation``.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

import numpy as np


BOARD = 10
MAX_UNITS = 40
MAX_ORDERS = 10
N_ITEMS = 12
N_PRODUCTS = 9
N_CROPS = 5
N_UNIT_OPS = 18
N_MARKET_OPS = 8
N_QUANTITIES = 102
ALL_AVAILABLE = 101
MARKET_END = 7

ITEM_NAMES = (
    "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG",
    "MILK", "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP",
)
UNIT_NAMES = (
    "PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP",
    "PLACE", "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG",
    "BUILD_COOP", "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE",
)
MARKET_NAMES = (
    "NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT",
    "BUY_ANIMAL", "SELL", "END",
)
SEED_COST = np.asarray((10, 20, 50, 100, 80), dtype=np.int64)
ANIMAL_COST = np.asarray((300, 400, 500), dtype=np.int64)
LAND_COST = np.asarray((1000, 2000, 4000), dtype=np.int64)
FIRST_YIELD_DAY = np.asarray((2, 2, 8, 10, 10), dtype=np.int64)

def _unbatch(value: np.ndarray) -> np.ndarray:
    value = np.asarray(value)
    return value[0] if value.ndim and value.shape[0] == 1 else value


def _argmax(logits: np.ndarray, mask: np.ndarray) -> int:
    logits = np.asarray(logits, dtype=np.float32)
    mask = np.asarray(mask, dtype=np.bool_)
    if logits.shape != mask.shape or not mask.any():
        raise ValueError(f"invalid logits/mask shapes {logits.shape}/{mask.shape}")
    if not np.isfinite(logits).all():
        raise FloatingPointError("non-finite action logits")
    return int(np.argmax(np.where(mask, logits, -np.inf)))


def pack_mask(mask: np.ndarray) -> np.int64:
    result = 0
    for index in np.flatnonzero(mask):
        result |= 1 << int(index)
    return np.int64(result)


def unpack_mask(bits: Any, width: int) -> np.ndarray:
    bits = np.asarray(bits, dtype=np.uint32)
    shifts = np.arange(width, dtype=np.uint32)
    return ((bits[..., None] >> shifts) & 1).astype(np.bool_)


def quantity_mask(maximum: int) -> np.ndarray:
    mask = np.zeros(N_QUANTITIES, dtype=np.bool_)
    if maximum <= 0:
        return mask
    mask[1:min(100, maximum) + 1] = True
    if maximum > 100:
        mask[ALL_AVAILABLE] = True
    return mask


def quantity_value(category: int, maximum: int) -> int:
    return maximum if category == ALL_AVAILABLE else min(maximum, max(1, category))


def _fib(index: int) -> int:
    a, b = 1, 1
    for _ in range(max(0, index)):
        a, b = b, a + b
    return a


@dataclass
class PlanningState:
    tiles: np.ndarray
    positions: np.ndarray
    active: np.ndarray
    shed: np.ndarray
    seeds: np.ndarray
    inventories: np.ndarray
    inventory_order: np.ndarray
    market_inventory: np.ndarray
    market_prices: np.ndarray
    money: int
    n_quadrants: int
    hires_today: int
    day: int
    price_function: Callable[[int, int], int] | None = None
    shed_capacity: int = 100
    stopped: bool = False

    @classmethod
    def from_observation(cls, obs: dict[str, np.ndarray], shed_capacity: int = 100,
                         price_function: Callable[[int, int], int] | None = None):
        inventories = _unbatch(obs["own_inventory"]).astype(np.int64, copy=True)
        if "own_inventory_order" in obs:
            inventory_order = _unbatch(obs["own_inventory_order"]).astype(
                np.int64, copy=True)
        else:
            # Compatibility with observations/checkpoints made before insertion
            # ranks became explicit. This fallback is deterministic but cannot
            # reconstruct historical dict order when several item kinds exist.
            inventory_order = np.zeros_like(inventories)
            for unit in range(len(inventories)):
                present = np.flatnonzero(inventories[unit] > 0)
                inventory_order[unit, present] = np.arange(1, len(present) + 1)
        return cls(
            _unbatch(obs["tiles"])[0].copy(),
            _unbatch(obs["positions"])[0].copy(),
            _unbatch(obs["active_units"])[0].astype(bool, copy=True),
            _unbatch(obs["own_shed"]).astype(np.int64, copy=True),
            _unbatch(obs["own_seeds"]).astype(np.int64, copy=True),
            inventories,
            inventory_order,
            _unbatch(obs["market"])[..., 0].astype(np.int64, copy=True),
            _unbatch(obs["market"])[..., 1].astype(np.int64, copy=True),
            int(_unbatch(obs["farm"])[0, 0]),
            int(_unbatch(obs["farm"])[0, 2]),
            int(_unbatch(obs["farm"])[0, 3]),
            int(_unbatch(obs["clock"])[1]),
            price_function,
            shed_capacity,
        )

    def _inventory_keys(self, unit: int) -> list[int]:
        present = np.flatnonzero((self.inventories[unit] > 0) &
                                 (self.inventory_order[unit] > 0))
        return sorted(map(int, present), key=lambda item: int(self.inventory_order[unit, item]))

    def _inv_add(self, unit: int, item: int, quantity: int) -> None:
        if quantity <= 0:
            return
        if int(self.inventories[unit, item]) == 0:
            self.inventory_order[unit, item] = int(self.inventory_order[unit].max()) + 1
        self.inventories[unit, item] += quantity

    def _inv_take(self, unit: int, item: int, quantity: int) -> bool:
        if quantity < 0 or int(self.inventories[unit, item]) < quantity:
            return False
        self.inventories[unit, item] -= quantity
        if int(self.inventories[unit, item]) == 0:
            rank = int(self.inventory_order[unit, item])
            self.inventory_order[unit, item] = 0
            if rank > 0:
                self.inventory_order[unit, self.inventory_order[unit] > rank] -= 1
        return True

    @property
    def shed_room(self) -> int:
        return max(0, self.shed_capacity - int(self.shed.sum()))

    def adjacent(self, unit: int) -> bool:
        return tuple(map(int, self.positions[unit])) in {(4, 4), (5, 4), (4, 5), (5, 5)}

    def unit_item_mask(self, unit: int, operation: int) -> np.ndarray:
        mask = np.zeros(N_ITEMS, dtype=np.bool_)
        if operation == 5 and self.adjacent(unit):                 # PICKUP
            mask = self.shed > 0
        elif operation == 7:                                      # PLACE
            if self.adjacent(unit) and self.shed_room > 0:
                mask |= self.inventories[unit] > 0
            x, y = map(int, self.positions[unit])
            tile = self.tiles[y, x]
            if int(tile[0]) == 3 and not bool(tile[2]):            # COOP
                mask[9] = self.inventories[unit, 9] > 0
            if int(tile[0]) == 4 and not bool(tile[2]):            # PASTURE
                mask[10:12] = self.inventories[unit, 10:12] > 0
        elif operation == 8:                                      # PLANT
            mask[:N_CROPS] = self.seeds > 0
        return mask

    def unit_quantity_max(self, unit: int, operation: int, item: int) -> int:
        if operation == 5:
            return int(self.shed[item])
        if operation == 7:
            if item >= 9 and int(self.tiles[tuple(self.positions[unit][::-1])][0]) in (3, 4):
                return 1
            return min(int(self.inventories[unit, item]), self.shed_room)
        return 1

    def unit_op_mask(self, unit: int) -> np.ndarray:
        mask = np.zeros(N_UNIT_OPS, dtype=np.bool_)
        mask[0] = True
        if unit >= len(self.active) or not self.active[unit]:
            return mask
        x, y = map(int, self.positions[unit])
        mask[1:5] = (y > 0, y < BOARD - 1, x < BOARD - 1, x > 0)
        if self.adjacent(unit):
            mask[5] = bool((self.shed > 0).any())
            mask[6] = bool((self.inventories[unit] > 0).any())
        tile = self.tiles[y, x]
        kind, has_animal = int(tile[0]), bool(tile[2])
        if self.unit_item_mask(unit, 7).any():
            mask[7] = True
        if kind == 0:
            mask[8] = bool((self.seeds > 0).any())
            mask[13:15] = True
        elif kind == 2:
            mask[12] = True
        elif kind == 5:
            crop = int(tile[1])
            mask[9] = not bool(tile[3])
            mask[10] = int(tile[8]) > 0 and self.day - int(tile[10]) >= FIRST_YIELD_DAY[crop]
            mask[11] = self.inventories[unit, 8] > 0
            mask[12] = True
        elif kind in (3, 4):
            if has_animal:
                mask[10] = int(tile[8]) > 0
                mask[15] = not bool(tile[4]) and self.inventories[unit, 0] > 0
                mask[16] = bool(tile[6])
                mask[17] = not bool(tile[5])
            else:
                mask[12] = True
        return mask

    def apply_unit(self, unit: int, operation: int, item: int = 0, quantity: int = 1) -> None:
        x, y = map(int, self.positions[unit])
        if operation in (1, 2, 3, 4):
            self.positions[unit] += {1: (0, -1), 2: (0, 1), 3: (1, 0), 4: (-1, 0)}[operation]
            return
        if operation == 5:
            count = min(quantity, int(self.shed[item]))
            self.shed[item] -= count
            self._inv_add(unit, item, count)
            return
        if operation == 6:
            for index in self._inventory_keys(unit):
                count = min(int(self.inventories[unit, index]), self.shed_room)
                self.shed[index] += count
                self._inv_take(unit, index, int(self.inventories[unit, index]))
            return
        tile = self.tiles[y, x]
        if operation == 7:
            if item >= 9 and int(tile[0]) in (3, 4) and not bool(tile[2]):
                self._inv_take(unit, item, 1)
                tile[:] = 0
                tile[0], tile[1], tile[2], tile[10] = (3 if item == 9 else 4), item, 1, self.day
            elif self.adjacent(unit):
                count = min(quantity, int(self.inventories[unit, item]), self.shed_room)
                self._inv_take(unit, item, count)
                self.shed[item] += count
        elif operation == 8:
            self.seeds[item] -= 1
            tile[:] = 0
            tile[0], tile[1], tile[7], tile[10] = 5, item, 1, self.day
            tile[8] = 0 if item in (2, 3) else 1
        elif operation == 9:
            tile[3] = 1
        elif operation == 10:
            product = int(tile[1]) if int(tile[0]) == 5 else {9: 5, 10: 6, 11: 7}.get(int(tile[1]), 0)
            self._inv_add(unit, product, int(tile[8]))
            tile[8] = 0
            if int(tile[0]) == 5 and int(tile[1]) not in (2, 3):
                tile[:] = 0
        elif operation == 11:
            self._inv_take(unit, 8, 1)
        elif operation == 12:
            tile[:] = 0
        elif operation in (13, 14):
            tile[:] = 0
            tile[0] = 3 if operation == 13 else 4
        elif operation == 15:
            self._inv_take(unit, 0, 1)
            tile[4] = 1
        elif operation == 16:
            self._inv_add(unit, 8, 1)
            tile[6] = 0
        elif operation == 17:
            tile[5] = 1

    def _market_item_max(self, operation: int, item: int) -> int:
        if operation == 3 and item < N_CROPS:
            return self._affordable(SEED_COST[item], 100)
        if operation == 4 and item in (0, 8):
            return self._affordable_product(item)
        if operation == 5 and item >= 9:
            return self._affordable(ANIMAL_COST[item - 9], self.shed_room)
        if operation == 6 and item < N_PRODUCTS:
            return int(self.shed[item])
        return 0

    def _affordable(self, price: int, capacity: int) -> int:
        if price <= 0:
            return 0
        return max(0, min(capacity, self.money // int(price)))

    def _affordable_product(self, item: int) -> int:
        money, inventory = self.money, int(self.market_inventory[item])
        accepted = 0
        for _ in range(min(self.shed_room, inventory, 100)):
            cost = self.price(item, inventory - 1)
            if money < cost:
                break
            money -= cost
            inventory -= 1
            accepted += 1
        return accepted

    def price(self, item: int, inventory: int) -> int:
        if self.price_function is not None:
            return int(self.price_function(item, inventory))
        # Current displayed price is safe for observations without an injected
        # engine helper, but production training/export always injects one.
        return int(self.market_prices[item])

    def market_item_mask(self, operation: int) -> np.ndarray:
        return np.asarray([self._market_item_max(operation, item) > 0
                           for item in range(N_ITEMS)], dtype=np.bool_)

    def market_op_mask(self) -> np.ndarray:
        mask = np.zeros(N_MARKET_OPS, dtype=np.bool_)
        mask[MARKET_END] = True
        if self.stopped:
            return mask
        mask[1] = bool(self.active.sum() < MAX_UNITS and self.money >= _fib(self.hires_today))
        extra = self.n_quadrants - 1
        mask[2] = bool(0 <= extra < 3 and self.money >= LAND_COST[extra])
        for operation in range(3, 7):
            mask[operation] = self.market_item_mask(operation).any()
        return mask

    def apply_market(self, operation: int, item: int = 0, quantity: int = 1) -> None:
        if operation == MARKET_END:
            self.stopped = True
        elif operation == 1:
            self.money -= _fib(self.hires_today)
            self.hires_today += 1
            free = np.flatnonzero(~self.active)
            if len(free):
                self.active[free[0]] = True
                self.positions[free[0]] = (4, 4)
        elif operation == 2:
            self.money -= int(LAND_COST[self.n_quadrants - 1])
            self.n_quadrants += 1
        elif operation == 3:
            self.money -= int(SEED_COST[item]) * quantity
            self.seeds[item] += quantity
        elif operation == 4:
            for _ in range(quantity):
                self.money -= self.price(item, int(self.market_inventory[item]) - 1)
                self.market_inventory[item] -= 1
                self.shed[item] += 1
        elif operation == 5:
            self.money -= int(ANIMAL_COST[item - 9]) * quantity
            self.shed[item] += quantity
        elif operation == 6:
            for _ in range(quantity):
                price = self.price(item, int(self.market_inventory[item]))
                self.money += price
                self.shed[item] -= 1
                if price > 1:
                    self.market_inventory[item] += 1


def teacher_masks(obs: dict[str, np.ndarray], action: dict[str, np.ndarray],
                  price_function: Callable[[int, int], int] | None = None,
                  shed_capacity: int = 100) -> dict[str, np.ndarray]:
    """Masks and bounds encountered while replaying one legal target action."""
    state = PlanningState.from_observation(
        obs, shed_capacity=shed_capacity, price_function=price_function)
    unit_op_bits = np.zeros(MAX_UNITS, dtype=np.int64)
    unit_item_bits = np.zeros(MAX_UNITS, dtype=np.int64)
    unit_quantity_max = np.ones(MAX_UNITS, dtype=np.int16)
    for unit in range(MAX_UNITS):
        op_mask = state.unit_op_mask(unit)
        operation = int(action["unit_op"][unit]) if unit < int(action["n_units"]) else 0
        if not op_mask[operation]:
            raise ValueError(f"canonical unit target is illegal: unit={unit} op={operation}")
        unit_op_bits[unit] = pack_mask(op_mask)
        item = int(action["unit_arg"][unit])
        if operation in (5, 7, 8):
            item_mask = state.unit_item_mask(unit, operation)
            if not item_mask[item]:
                raise ValueError(
                    f"canonical unit item is illegal: unit={unit} op={operation} item={item}")
            unit_item_bits[unit] = pack_mask(item_mask)
            maximum = state.unit_quantity_max(unit, operation, item)
            if int(action["unit_n"][unit]) > maximum:
                raise ValueError(f"canonical unit quantity exceeds bound: "
                                 f"unit={unit} target={action['unit_n'][unit]} max={maximum}")
            unit_quantity_max[unit] = min(32767, maximum)
        if state.active[unit]:
            state.apply_unit(unit, operation, item, int(action["unit_n"][unit]))

    market_op_bits = np.zeros(MAX_ORDERS, dtype=np.int64)
    market_item_bits = np.zeros(MAX_ORDERS, dtype=np.int64)
    market_quantity_max = np.ones(MAX_ORDERS, dtype=np.int16)
    count = min(int(action["n_orders"]), MAX_ORDERS)
    for slot in range(MAX_ORDERS):
        operation = int(action["order_op"][slot]) if slot < count else MARKET_END
        op_mask = state.market_op_mask()
        if not op_mask[operation]:
            raise ValueError(f"canonical market target is illegal: slot={slot} op={operation}")
        market_op_bits[slot] = pack_mask(op_mask)
        if operation in (3, 4, 5, 6):
            item = int(action["order_item"][slot])
            item_mask = state.market_item_mask(operation)
            if not item_mask[item]:
                raise ValueError(
                    f"canonical market item is illegal: slot={slot} op={operation} item={item}")
            market_item_bits[slot] = pack_mask(item_mask)
            maximum = state._market_item_max(operation, item)
            if int(action["order_n"][slot]) > maximum:
                raise ValueError(f"canonical market quantity exceeds bound: "
                                 f"slot={slot} op={operation} item={item} "
                                 f"target={action['order_n'][slot]} max={maximum} "
                                 f"money={state.money} shed={int(state.shed[item])} "
                                 f"shed_room={state.shed_room}")
            market_quantity_max[slot] = min(32767, maximum)
        if slot < count:
            state.apply_market(operation, int(action["order_item"][slot]),
                               int(action["order_n"][slot]))
        else:
            state.apply_market(MARKET_END)
    return {
        "unit_op_bits": unit_op_bits, "unit_item_bits": unit_item_bits,
        "unit_quantity_max": unit_quantity_max,
        "market_op_bits": market_op_bits, "market_item_bits": market_item_bits,
        "market_quantity_max": market_quantity_max,
    }


def decode_action(output: dict[str, Any], obs: dict[str, np.ndarray],
                  price_function: Callable[[int, int], int] | None = None,
                  shed_capacity: int = 100) -> dict[str, Any]:
    """Greedily decode structured logits with exactly the masks used by BC."""
    state = PlanningState.from_observation(
        obs, shed_capacity=shed_capacity, price_function=price_function)
    unit_actions: list[list[Any]] = []
    active_units = int(state.active.sum())
    for unit in range(active_units):
        operation = _argmax(output["unit_op_logits"][0, unit], state.unit_op_mask(unit))
        item, quantity = 0, 1
        action: list[Any] = [UNIT_NAMES[operation]]
        if operation in (5, 7, 8):
            item = _argmax(output["unit_item_logits"][0, unit, operation],
                           state.unit_item_mask(unit, operation))
            action.append(ITEM_NAMES[item])
        if operation in (5, 7):
            maximum = state.unit_quantity_max(unit, operation, item)
            category = _argmax(output["unit_quantity_logits"][0, unit], quantity_mask(maximum))
            quantity = quantity_value(category, maximum)
            action.append(quantity)
        unit_actions.append(action)
        state.apply_unit(unit, operation, item, quantity)

    market_actions: list[list[Any]] = []
    hidden = output["market_hidden"]
    op_logits = output["market_op_logits"]
    item_logits = output["market_item_logits"]
    quantity_logits = output["market_quantity_logits"]
    for _slot in range(MAX_ORDERS):
        operation = _argmax(op_logits[0], state.market_op_mask())
        item, quantity = 0, 1
        if operation == MARKET_END:
            break
        action = [MARKET_NAMES[operation]]
        if operation in (3, 4, 5, 6):
            item = _argmax(item_logits[0], state.market_item_mask(operation))
            maximum = state._market_item_max(operation, item)
            category = _argmax(quantity_logits[0], quantity_mask(maximum))
            quantity = quantity_value(category, maximum)
            action.extend((ITEM_NAMES[item], quantity))
        market_actions.append(action)
        state.apply_market(operation, item, quantity)
        next_output = output["market_step"](hidden, operation, item)
        hidden = next_output["hidden"]
        op_logits = next_output["op_logits"]
        item_logits = next_output["item_logits"]
        quantity_logits = next_output["quantity_logits"]
    return {
        "farmer": unit_actions[0] if unit_actions else ["PASS"],
        "hands": unit_actions[1:],
        "market": market_actions,
    }


__all__ = [
    "ALL_AVAILABLE", "MARKET_END", "PlanningState", "decode_action",
    "pack_mask", "quantity_mask", "quantity_value",
    "teacher_masks", "unpack_mask",
]
