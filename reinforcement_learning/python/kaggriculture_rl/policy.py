"""Structured stochastic policy shared by self-play rollout and PPO scoring.

The sampler and evaluator consume the same path-dependent legality masks.  The
sampler stores those masks alongside the executed action; PPO never reconstructs
an approximation of the behavior distribution after the fact.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import numpy as np
import torch
from torch import Tensor

from . import schema
from .action_codec import PlanningState, pack_mask, quantity_value


ITEM_UNIT_OPS = (5, 7, 8)
QUANTITY_UNIT_OPS = (5, 7)
ITEM_MARKET_OPS = (3, 4, 5, 6)


@dataclass
class ActionBatch:
    unit_op: np.ndarray
    unit_arg: np.ndarray
    unit_n: np.ndarray
    n_units: np.ndarray
    order_op: np.ndarray
    order_item: np.ndarray
    order_n: np.ndarray
    n_orders: np.ndarray

    def arrays(self) -> dict[str, np.ndarray]:
        return {name: getattr(self, name) for name in self.__dataclass_fields__}


@dataclass
class MaskBatch:
    unit_op_bits: np.ndarray
    unit_item_bits: np.ndarray
    unit_quantity_max: np.ndarray
    market_op_bits: np.ndarray
    market_item_bits: np.ndarray
    market_quantity_max: np.ndarray

    def arrays(self) -> dict[str, np.ndarray]:
        return {name: getattr(self, name) for name in self.__dataclass_fields__}


@dataclass
class PolicyBatch:
    actions: ActionBatch
    masks: MaskBatch
    log_prob: np.ndarray
    entropy: np.ndarray
    state: Any


def _choose(logits: np.ndarray, mask: np.ndarray, deterministic: bool,
            rng: np.random.Generator, temperature: float) -> tuple[int, float, float]:
    mask = np.asarray(mask, dtype=np.bool_)
    if not mask.any() or logits.shape != mask.shape:
        raise ValueError(f"invalid categorical mask {logits.shape}/{mask.shape}")
    values = np.asarray(logits, dtype=np.float64) / temperature
    values = np.where(mask, values, -np.inf)
    if not np.isfinite(values[mask]).all():
        raise FloatingPointError("non-finite legal policy logits")
    peak = float(values[mask].max())
    probabilities = np.zeros_like(values)
    probabilities[mask] = np.exp(values[mask] - peak)
    probabilities /= probabilities.sum()
    choice = int(np.argmax(values) if deterministic else rng.choice(len(values), p=probabilities))
    log_probability = float(np.log(max(probabilities[choice], np.finfo(np.float64).tiny)))
    positive = probabilities > 0
    entropy = float(-(probabilities[positive] * np.log(probabilities[positive])).sum())
    return choice, log_probability, entropy


def _numpy(value: Tensor) -> np.ndarray:
    return value.detach().float().cpu().numpy()


@torch.inference_mode()
def act_batch(model, observations: dict[str, np.ndarray], state, *,
              device: torch.device, rng: np.random.Generator,
              deterministic: bool = False, temperature: float = 1.0,
              price_function=None, shed_capacity: int = 100) -> PolicyBatch:
    """Sample one executable joint action per observation row."""
    if temperature <= 0:
        raise ValueError("temperature must be positive")
    batch = int(observations["clock"].shape[0])
    torch_obs = {name: torch.from_numpy(value).to(device)
                 for name, value in observations.items()}
    with torch.autocast(device_type=device.type, dtype=torch.bfloat16,
                        enabled=device.type == "cuda"):
        output = model(torch_obs, state)

    actions = ActionBatch(
        np.zeros((batch, schema.MAX_UNITS), dtype=np.uint8),
        np.zeros((batch, schema.MAX_UNITS), dtype=np.uint8),
        np.ones((batch, schema.MAX_UNITS), dtype=np.int32),
        np.ones(batch, dtype=np.int32),
        np.zeros((batch, schema.MAX_ORDERS), dtype=np.uint8),
        np.zeros((batch, schema.MAX_ORDERS), dtype=np.uint8),
        np.zeros((batch, schema.MAX_ORDERS), dtype=np.int32),
        np.zeros(batch, dtype=np.int32),
    )
    masks = MaskBatch(
        np.ones((batch, schema.MAX_UNITS), dtype=np.int64),
        np.zeros((batch, schema.MAX_UNITS), dtype=np.int64),
        np.ones((batch, schema.MAX_UNITS), dtype=np.int16),
        np.full((batch, schema.MAX_ORDERS),
                1 << schema.MARKET_END, dtype=np.int64),
        np.zeros((batch, schema.MAX_ORDERS), dtype=np.int64),
        np.ones((batch, schema.MAX_ORDERS), dtype=np.int16),
    )
    log_prob = np.zeros(batch, dtype=np.float32)
    entropy = np.zeros(batch, dtype=np.float32)
    planning = [
        PlanningState.from_observation(
            {name: value[row:row + 1] for name, value in observations.items()},
            shed_capacity=shed_capacity, price_function=price_function)
        for row in range(batch)
    ]
    unit_op_logits = _numpy(output["unit_op_logits"])
    unit_item_logits = _numpy(output["unit_item_logits"])
    unit_quantity_logits = _numpy(output["unit_quantity_logits"])
    for row, plan in enumerate(planning):
        actions.n_units[row] = int(plan.active.sum())
        for unit in range(actions.n_units[row]):
            op_mask = plan.unit_op_mask(unit)
            masks.unit_op_bits[row, unit] = pack_mask(op_mask)
            operation, lp, ent = _choose(
                unit_op_logits[row, unit], op_mask, deterministic, rng, temperature)
            actions.unit_op[row, unit] = operation
            log_prob[row] += lp
            entropy[row] += ent
            item, quantity = 0, 1
            if operation in ITEM_UNIT_OPS:
                item_mask = plan.unit_item_mask(unit, operation)
                masks.unit_item_bits[row, unit] = pack_mask(item_mask)
                item, lp, ent = _choose(
                    unit_item_logits[row, unit, operation], item_mask,
                    deterministic, rng, temperature)
                actions.unit_arg[row, unit] = item
                log_prob[row] += lp
                entropy[row] += ent
            if operation in QUANTITY_UNIT_OPS:
                maximum = plan.unit_quantity_max(unit, operation, item)
                masks.unit_quantity_max[row, unit] = maximum
                quantity_mask = np.zeros(schema.N_QUANTITIES, dtype=np.bool_)
                quantity_mask[1:min(100, maximum) + 1] = True
                if maximum > 100:
                    quantity_mask[schema.ALL_AVAILABLE] = True
                category, lp, ent = _choose(
                    unit_quantity_logits[row, unit], quantity_mask,
                    deterministic, rng, temperature)
                quantity = quantity_value(category, maximum)
                actions.unit_n[row, unit] = quantity
                log_prob[row] += lp
                entropy[row] += ent
            plan.apply_unit(unit, operation, item, quantity)

    hidden = output["market_hidden"]
    op_logits = output["market_op_logits"]
    item_logits = output["market_item_logits"]
    quantity_logits = output["market_quantity_logits"]
    live = np.ones(batch, dtype=np.bool_)
    for slot in range(schema.MAX_ORDERS):
        op_values, item_values, quantity_values = map(
            _numpy, (op_logits, item_logits, quantity_logits))
        feedback_op = np.full(batch, schema.MARKET_END, dtype=np.int64)
        feedback_item = np.zeros(batch, dtype=np.int64)
        for row, plan in enumerate(planning):
            if not live[row]:
                continue
            op_mask = plan.market_op_mask()
            masks.market_op_bits[row, slot] = pack_mask(op_mask)
            operation, lp, ent = _choose(
                op_values[row], op_mask, deterministic, rng, temperature)
            log_prob[row] += lp
            entropy[row] += ent
            feedback_op[row] = operation
            if operation == schema.MARKET_END:
                live[row] = False
                plan.apply_market(schema.MARKET_END)
                continue
            item, quantity = 0, 1
            if operation in ITEM_MARKET_OPS:
                item_mask = plan.market_item_mask(operation)
                masks.market_item_bits[row, slot] = pack_mask(item_mask)
                item, lp, ent = _choose(
                    item_values[row], item_mask, deterministic, rng, temperature)
                maximum = plan._market_item_max(operation, item)
                masks.market_quantity_max[row, slot] = maximum
                quantity_mask = np.zeros(schema.N_QUANTITIES, dtype=np.bool_)
                quantity_mask[1:min(100, maximum) + 1] = True
                if maximum > 100:
                    quantity_mask[schema.ALL_AVAILABLE] = True
                category, qlp, qent = _choose(
                    quantity_values[row], quantity_mask, deterministic, rng, temperature)
                quantity = quantity_value(category, maximum)
                log_prob[row] += lp + qlp
                entropy[row] += ent + qent
            actions.order_op[row, slot] = operation
            actions.order_item[row, slot] = item
            actions.order_n[row, slot] = quantity
            actions.n_orders[row] += 1
            feedback_item[row] = item
            plan.apply_market(operation, item, quantity)
        if not live.any():
            break
        with torch.autocast(device_type=device.type, dtype=torch.bfloat16,
                            enabled=device.type == "cuda"):
            next_output = model.market_step(
                hidden, torch.from_numpy(feedback_op).to(device),
                torch.from_numpy(feedback_item).to(device))
        hidden = next_output["hidden"]
        op_logits = next_output["op_logits"]
        item_logits = next_output["item_logits"]
        quantity_logits = next_output["quantity_logits"]
    if live.any():
        # Ten real orders fill the engine limit; there is no transmitted END.
        live[:] = False
    return PolicyBatch(actions, masks, log_prob, entropy, output["state"])


def joint_engine_arrays(first: ActionBatch, second: ActionBatch) -> tuple[np.ndarray, ...]:
    """Stack two perspective-relative batches into VectorEnv.step order."""
    return (
        np.stack((first.unit_op, second.unit_op), axis=1),
        np.stack((first.unit_arg, second.unit_arg), axis=1),
        np.stack((first.unit_n, second.unit_n), axis=1),
        np.stack((first.n_units, second.n_units), axis=1),
        np.stack((first.order_op, second.order_op), axis=1),
        np.stack((first.order_item, second.order_item), axis=1),
        np.stack((first.order_n, second.order_n), axis=1),
        np.stack((first.n_orders, second.n_orders), axis=1),
    )


def _unpack_bits(bits: Tensor, width: int) -> Tensor:
    shifts = torch.arange(width, device=bits.device, dtype=torch.int64)
    return ((bits.to(torch.int64).unsqueeze(-1) >> shifts) & 1).bool()


def _categorical_stats(logits: Tensor, mask: Tensor, target: Tensor,
                       relevant: Tensor, temperature: float) -> tuple[Tensor, Tensor]:
    safe_mask = mask.clone()
    safe_target = target.clone()
    irrelevant = ~relevant
    safe_mask[irrelevant] = False
    safe_mask[irrelevant, 0] = True
    safe_target[irrelevant] = 0
    log_probs = torch.log_softmax(
        (logits / temperature).masked_fill(~safe_mask, -torch.inf), dim=-1)
    selected = log_probs.gather(-1, safe_target[:, None]).squeeze(-1)
    probabilities = torch.softmax(
        (logits / temperature).masked_fill(~safe_mask, -torch.inf), dim=-1)
    finite_log_probs = torch.where(safe_mask, log_probs, torch.zeros_like(log_probs))
    terms = probabilities * finite_log_probs
    entropy = -terms.sum(dim=-1)
    return selected * relevant, entropy * relevant


def evaluate_actions(model, observations: dict[str, Tensor], state,
                     actions: dict[str, Tensor], masks: dict[str, Tensor],
                     *, temperature: float = 1.0):
    """Recompute exact joint log probability for actions sampled by act_batch."""
    output = model(observations, state)
    batch = observations["clock"].shape[0]
    ids = torch.arange(schema.MAX_UNITS, device=observations["clock"].device)[None]
    active = ids < actions["n_units"][:, None]
    op_mask = _unpack_bits(masks["unit_op_bits"], schema.N_UNIT_OPS)
    lp, ent = _categorical_stats(
        output["unit_op_logits"].reshape(-1, schema.N_UNIT_OPS),
        op_mask.reshape(-1, schema.N_UNIT_OPS),
        actions["unit_op"].long().reshape(-1), active.reshape(-1), temperature)
    log_prob = lp.reshape(batch, -1).sum(-1)
    entropy = ent.reshape(batch, -1).sum(-1)

    op = actions["unit_op"].long()
    item_relevant = active & ((op == 5) | (op == 7) | (op == 8))
    rows = torch.arange(batch, device=op.device)[:, None]
    units = torch.arange(schema.MAX_UNITS, device=op.device)[None]
    item_logits = output["unit_item_logits"][rows, units, op]
    item_mask = _unpack_bits(masks["unit_item_bits"], schema.N_ITEMS)
    lp, ent = _categorical_stats(
        item_logits.reshape(-1, schema.N_ITEMS), item_mask.reshape(-1, schema.N_ITEMS),
        actions["unit_arg"].long().reshape(-1), item_relevant.reshape(-1), temperature)
    log_prob += lp.reshape(batch, -1).sum(-1)
    entropy += ent.reshape(batch, -1).sum(-1)

    quantity_relevant = active & ((op == 5) | (op == 7))
    maximum = masks["unit_quantity_max"].long()
    classes = torch.arange(schema.N_QUANTITIES, device=op.device)
    quantity_mask = ((classes[None, None] >= 1) &
                     (classes[None, None] <= torch.minimum(
                         maximum[:, :, None], maximum.new_tensor(100)))) | (
        (classes[None, None] == schema.ALL_AVAILABLE) & (maximum[:, :, None] > 100))
    quantity_target = torch.where(
        actions["unit_n"] > 100,
        actions["unit_n"].new_full((), schema.ALL_AVAILABLE),
        actions["unit_n"]).long()
    lp, ent = _categorical_stats(
        output["unit_quantity_logits"].reshape(-1, schema.N_QUANTITIES),
        quantity_mask.reshape(-1, schema.N_QUANTITIES),
        quantity_target.reshape(-1), quantity_relevant.reshape(-1), temperature)
    log_prob += lp.reshape(batch, -1).sum(-1)
    entropy += ent.reshape(batch, -1).sum(-1)

    hidden = output["market_hidden"]
    op_logits = output["market_op_logits"]
    item_logits = output["market_item_logits"]
    quantity_logits = output["market_quantity_logits"]
    for slot in range(schema.MAX_ORDERS):
        valid = actions["n_orders"] >= slot
        real = actions["n_orders"] > slot
        target_op = torch.where(
            real, actions["order_op"][:, slot].long(),
            actions["order_op"].new_full((batch,), schema.MARKET_END).long())
        market_op_mask = _unpack_bits(masks["market_op_bits"][:, slot],
                                      schema.N_MARKET_OPS)
        lp, ent = _categorical_stats(
            op_logits, market_op_mask, target_op, valid, temperature)
        log_prob += lp
        entropy += ent
        relevant = real & ((target_op >= 3) & (target_op <= 6))
        market_item_mask = _unpack_bits(
            masks["market_item_bits"][:, slot], schema.N_ITEMS)
        target_item = actions["order_item"][:, slot].long()
        lp, ent = _categorical_stats(
            item_logits, market_item_mask, target_item, relevant, temperature)
        log_prob += lp
        entropy += ent
        maximum = masks["market_quantity_max"][:, slot].long()
        quantity_mask = ((classes[None] >= 1) &
                         (classes[None] <= torch.minimum(
                             maximum[:, None], maximum.new_tensor(100)))) | (
            (classes[None] == schema.ALL_AVAILABLE) & (maximum[:, None] > 100))
        quantity_target = torch.where(
            actions["order_n"][:, slot] > 100,
            actions["order_n"].new_full((), schema.ALL_AVAILABLE),
            actions["order_n"][:, slot]).long()
        lp, ent = _categorical_stats(
            quantity_logits, quantity_mask, quantity_target, relevant, temperature)
        log_prob += lp
        entropy += ent
        next_output = model.market_step(hidden, target_op, target_item)
        hidden = next_output["hidden"]
        op_logits = next_output["op_logits"]
        item_logits = next_output["item_logits"]
        quantity_logits = next_output["quantity_logits"]
    return log_prob, entropy, output["state"]


__all__ = [
    "ActionBatch", "MaskBatch", "PolicyBatch", "act_batch",
    "evaluate_actions", "joint_engine_arrays",
]
