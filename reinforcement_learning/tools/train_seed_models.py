#!/usr/bin/env python3
"""Train independently growable, legality-aware behavior-clone seed models."""

from __future__ import annotations

import argparse
import copy
from dataclasses import asdict
import hashlib
import json
from pathlib import Path
import random
import re
import time
import math

import numpy as np
import torch
from torch import nn

from kaggriculture_rl import (KaggricultureActor, ModelConfig, Progress,
                              ReplayCursor, market_price)
from kaggriculture_rl import schema
from kaggriculture_rl.action_codec import teacher_masks


ITEM_UNIT_OPS = {schema.UnitOp.PICKUP, schema.UnitOp.PLACE, schema.UnitOp.PLANT}
QUANTITY_UNIT_OPS = {schema.UnitOp.PICKUP, schema.UnitOp.PLACE}
ITEM_MARKET_OPS = {
    schema.MarketOp.BUY_SEED, schema.MarketOp.BUY_PRODUCT,
    schema.MarketOp.BUY_ANIMAL, schema.MarketOp.SELL,
}


def slug(value: str) -> str:
    base = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-") or "agent"
    suffix = hashlib.sha1(value.encode()).hexdigest()[:8]
    return f"{base[:40]}-{suffix}"


def replay_fingerprints(cursor, stride: int = 4):
    """Coarse phase/action signatures used only when submission IDs are absent."""
    width = 3 * (schema.N_UNIT_OPS + 7 + schema.N_ITEMS)
    values = np.zeros((2, width), dtype=np.float32)
    while cursor.step + 1 < cursor.n_steps:
        if cursor.step % stride == 0:
            phase = min(2, 3 * cursor.step // max(1, cursor.n_steps - 1))
            base = phase * (schema.N_UNIT_OPS + 7 + schema.N_ITEMS)
            for seat in (0, 1):
                action = cursor.recorded_action(seat)
                for unit in range(min(int(action["n_units"]), schema.MAX_UNITS)):
                    operation = int(action["unit_op"][unit])
                    if 0 <= operation < schema.N_UNIT_OPS:
                        values[seat, base + operation] += 1
                market_base = base + schema.N_UNIT_OPS
                item_base = market_base + 7
                for order in range(min(int(action["n_orders"]), schema.MAX_ORDERS)):
                    operation = int(action["order_op"][order])
                    item = int(action["order_item"][order])
                    if 0 <= operation < 7:
                        values[seat, market_base + operation] += 1
                    if 0 <= item < schema.N_ITEMS and operation != 0:
                        values[seat, item_base + item] += 1
        cursor.advance()
    for seat in (0, 1):
        for phase in range(3):
            begin = phase * (schema.N_UNIT_OPS + 7 + schema.N_ITEMS)
            boundaries = (begin, begin + schema.N_UNIT_OPS,
                          begin + schema.N_UNIT_OPS + 7,
                          begin + schema.N_UNIT_OPS + 7 + schema.N_ITEMS)
            for left, right in zip(boundaries, boundaries[1:]):
                total = values[seat, left:right].sum()
                if total:
                    values[seat, left:right] /= total
    return values


def discover(paths: list[Path], selected_teams: set[str] | None = None,
             compute_fingerprints: bool = True):
    groups: dict[str, list[tuple[Path, int]]] = {}
    metadata = []
    progress = Progress("discover replays", len(paths))
    for index, path in enumerate(paths, 1):
        cursor = ReplayCursor(str(path))
        fingerprints = (replay_fingerprints(cursor) if compute_fingerprints
                        else (None, None))
        for seat, team in enumerate(cursor.teams):
            if selected_teams and team not in selected_teams:
                continue
            submission_ids = list(cursor.submission_ids)
            submission = submission_ids[seat] if seat < len(submission_ids) else 0
            cluster = f"{team} [submission {submission}]" if submission else team
            groups.setdefault(cluster, []).append((path, seat))
            metadata.append({"path": str(path), "seat": seat, "team": team,
                             "cluster": cluster, "submission_id": submission,
                             "fingerprint": fingerprints[seat],
                             "reward": cursor.rewards[seat],
                             "opponent_reward": cursor.rewards[1 - seat]})
        progress.update(index, detail=path.name)
    return groups, metadata


def _kmeans_two(rows, seed):
    values = np.stack([row["fingerprint"] for row in rows])
    centre = values.mean(axis=0)
    first = int(np.argmax(((values - centre) ** 2).sum(axis=1)))
    second = int(np.argmax(((values - values[first]) ** 2).sum(axis=1)))
    centroids = np.stack((values[first], values[second]))
    labels = np.zeros(len(rows), dtype=np.int64)
    for _ in range(30):
        distances = ((values[:, None] - centroids[None]) ** 2).sum(axis=2)
        updated = distances.argmin(axis=1)
        if np.array_equal(updated, labels) and _:
            break
        labels = updated
        for cluster in (0, 1):
            if np.any(labels == cluster):
                centroids[cluster] = values[labels == cluster].mean(axis=0)
    separation = float(np.linalg.norm(centroids[0] - centroids[1]))
    return labels, separation


def split_behavior_versions(groups, metadata, minimum: int, seed: int):
    """Split large team-only groups when two stable behavior modes are evident."""
    by_key = {(row["path"], row["seat"]): row for row in metadata}
    result = {}
    for name, members in groups.items():
        rows = [by_key[(str(path), seat)] for path, seat in members]
        has_submission = any(row["submission_id"] for row in rows)
        if has_submission or len(rows) < 2 * minimum:
            result[name] = members
            continue
        labels, separation = _kmeans_two(rows, seed + len(result))
        counts = np.bincount(labels, minlength=2)
        if separation < 0.08 or counts.min() < minimum:
            result[name] = members
            continue
        for cluster in (0, 1):
            cluster_name = f"{name} [behavior {cluster + 1}]"
            result[cluster_name] = [member for member, label in zip(members, labels)
                                    if label == cluster]
            for row, label in zip(rows, labels):
                if label == cluster:
                    row["cluster"] = cluster_name
    return result


def apply_manifest(groups, path: Path | None):
    if path is None:
        return groups
    specification = json.loads(path.read_text())
    assigned: dict[str, list[tuple[Path, int]]] = {}
    lookup = {(str(p), seat): (p, seat) for members in groups.values() for p, seat in members}
    for cluster, rows in specification["clusters"].items():
        for row in rows:
            key = (row["path"], int(row["seat"]))
            if key not in lookup:
                raise ValueError(f"manifest trajectory is not in replay input: {key}")
            assigned.setdefault(cluster, []).append(lookup[key])
    return assigned


def encode_quantity(value: np.ndarray) -> np.ndarray:
    value = value.astype(np.int64, copy=False)
    return np.where(value > 100, schema.ALL_AVAILABLE, np.clip(value, 0, 100))


def canonical_policy_action(cursor, seat: int):
    """Observable, executable label with compact market order positions.

    Unit execution is independent across players, so the joint canonicalizer is
    used only to recover accepted PICKUP/PLACE quantities.  Market labels come
    from the PASS-opponent sanitizer and therefore never encode unavailable
    knowledge about the opponent's simultaneous order.
    """
    units = cursor.joint_effective_action(seat)
    market = cursor.solo_effective_action(seat)
    result = {key: np.array(value, copy=True) if isinstance(value, np.ndarray) else value
              for key, value in units.items()}
    kept = [index for index in range(min(int(market["n_orders"]), schema.MAX_ORDERS))
            if int(market["order_op"][index]) != int(schema.MarketOp.NONE)
            and int(market["order_n"][index]) > 0]
    result["order_op"][:] = int(schema.MarketOp.NONE)
    result["order_item"][:] = 0
    result["order_n"][:] = 0
    for destination, source in enumerate(kept):
        result["order_op"][destination] = market["order_op"][source]
        result["order_item"][destination] = market["order_item"][source]
        result["order_n"][destination] = market["order_n"][source]
    result["n_orders"] = len(kept)
    return result


def load_trajectory(path: Path, seat: int, stride: int):
    cursor = ReplayCursor(str(path))
    if not cursor.verify_joint_canonicalization():
        raise RuntimeError(f"joint canonicalization parity failed: {path}")
    observations: dict[str, list[np.ndarray]] = {}
    labels: dict[str, list[np.ndarray | int | float]] = {
        "unit_op": [], "unit_item": [], "unit_quantity": [],
        "market_op": [], "market_item": [], "market_quantity": [],
        "market_valid": [], "wdl": [], "margin": [], "opponent_flow": [],
        "unit_op_bits": [], "unit_item_bits": [], "unit_quantity_max": [],
        "market_op_bits": [], "market_item_bits": [], "market_quantity_max": [],
    }
    margin = cursor.rewards[seat] - cursor.rewards[1 - seat]
    while cursor.step + 1 < cursor.n_steps:
        if cursor.step % stride == 0:
            obs = cursor.observe(seat)
            for name, value in obs.items():
                observations.setdefault(name, []).append(value[0])
            # The actor can reproduce only action effects determined from its
            # own observation.  Active-opponent resolution belongs in the
            # transition/value target, never in the policy label.
            action = canonical_policy_action(cursor, seat)
            try:
                masks = teacher_masks(obs, action, market_price, cursor.shed_capacity)
            except ValueError as error:
                units = [(index, int(action["unit_op"][index]),
                          int(action["unit_arg"][index]), int(action["unit_n"][index]))
                         for index in range(int(action["n_units"]))
                         if int(action["unit_op"][index])]
                orders = [(int(action["order_op"][index]),
                           int(action["order_item"][index]), int(action["order_n"][index]))
                          for index in range(min(int(action["n_orders"]), schema.MAX_ORDERS))]
                raise ValueError(
                    f"{path}:seat{seat}:step{cursor.step}: {error}; "
                    f"units={units}; orders={orders}") from error
            labels["unit_op"].append(action["unit_op"].astype(np.int64))
            labels["unit_item"].append(action["unit_arg"].astype(np.int64))
            labels["unit_quantity"].append(encode_quantity(action["unit_n"]))
            count = min(int(action["n_orders"]), schema.MAX_ORDERS)
            ops = np.full(schema.MAX_ORDERS, schema.MARKET_END, dtype=np.int64)
            items = np.zeros(schema.MAX_ORDERS, dtype=np.int64)
            quantities = np.zeros(schema.MAX_ORDERS, dtype=np.int64)
            valid = np.zeros(schema.MAX_ORDERS, dtype=bool)
            if count:
                ops[:count] = action["order_op"][:count]
                items[:count] = action["order_item"][:count]
                quantities[:count] = encode_quantity(action["order_n"][:count])
                valid[:count] = True
            if count < schema.MAX_ORDERS:
                valid[count] = True  # one decoder-only END target
            labels["market_op"].append(ops)
            labels["market_item"].append(items)
            labels["market_quantity"].append(quantities)
            labels["market_valid"].append(valid)
            labels["wdl"].append(cursor.target(seat) + 1)
            labels["margin"].append(np.sign(margin) * np.log1p(abs(margin)))
            opponent = cursor.solo_effective_action(1 - seat)
            flow = np.zeros(schema.N_PRODUCTS, dtype=np.float32)
            for order in range(min(int(opponent["n_orders"]), schema.MAX_ORDERS)):
                operation = int(opponent["order_op"][order])
                item = int(opponent["order_item"][order])
                if item < schema.N_PRODUCTS and operation in (schema.MarketOp.BUY_PRODUCT,
                                                               schema.MarketOp.SELL):
                    direction = -1.0 if operation == schema.MarketOp.BUY_PRODUCT else 1.0
                    flow[item] += direction * float(opponent["order_n"][order])
            labels["opponent_flow"].append(np.sign(flow) * np.log1p(np.abs(flow)))
            for name, value in masks.items():
                labels[name].append(value)
        cursor.advance()
    return ({name: np.stack(values) for name, values in observations.items()},
            {name: np.asarray(values) for name, values in labels.items()})


class TrajectorySet:
    def __init__(self, members, stride: int, label: str = "load trajectories"):
        progress = Progress(label, len(members))
        self.trajectories = []
        for index, (path, seat) in enumerate(members, 1):
            self.trajectories.append(load_trajectory(path, seat, stride))
            progress.update(index, detail=f"{path.name}:seat{seat}")
        self.windows = []
        self.strategic_windows = []
        self.late_windows = []

    def build_windows(self, length: int):
        self.windows = []
        self.strategic_windows = []
        self.late_windows = []
        for trajectory, (obs, _) in enumerate(self.trajectories):
            steps = len(obs["clock"])
            if steps < length:
                continue
            labels = self.trajectories[trajectory][1]
            for start in range(steps - length + 1):
                reference = (trajectory, start)
                self.windows.append(reference)
                stop = start + length
                unit_ops = labels["unit_op"][start:stop]
                market_ops = labels["market_op"][start:stop]
                if np.any(unit_ops >= int(schema.UnitOp.PICKUP)) or np.any(
                        (market_ops > int(schema.MarketOp.NONE)) &
                        (market_ops < schema.MARKET_END)):
                    self.strategic_windows.append(reference)
                if int(obs["clock"][start, 0]) >= 576:
                    self.late_windows.append(reference)
        if not self.windows:
            raise ValueError("no trajectory is long enough for the requested sequence length")

    def batch(self, size: int, length: int, rng: random.Random, device):
        refs = [self.sample_ref(rng) for _ in range(size)]
        return self._collate(refs, length, device)

    def sample_ref(self, rng: random.Random):
        draw = rng.random()
        if draw < 0.35 and self.strategic_windows:
            return rng.choice(self.strategic_windows)
        if draw < 0.60 and self.late_windows:
            return rng.choice(self.late_windows)
        return rng.choice(self.windows)

    def _collate(self, refs, length: int, device):
        obs_batch, label_batch = {}, {}
        for trajectory, start in refs:
            obs, labels = self.trajectories[trajectory]
            for name, value in obs.items():
                obs_batch.setdefault(name, []).append(value[start:start + length])
            for name, value in labels.items():
                label_batch.setdefault(name, []).append(value[start:start + length])
        obs = {name: torch.from_numpy(np.stack(value)).to(device)
               for name, value in obs_batch.items()}
        labels = {name: torch.from_numpy(np.stack(value)).to(device)
                  for name, value in label_batch.items()}
        return obs, labels


class ClusterBalancedSet:
    """Sample lineages uniformly, then sample a window within the lineage."""

    def __init__(self, datasets):
        self.datasets = list(datasets)
        if not self.datasets:
            raise ValueError("at least one trajectory set is required")

    def batch(self, size: int, length: int, rng: random.Random, device):
        obs_batch, label_batch = {}, {}
        for _ in range(size):
            dataset = rng.choice(self.datasets)
            trajectory, start = dataset.sample_ref(rng)
            obs, labels = dataset.trajectories[trajectory]
            for name, value in obs.items():
                obs_batch.setdefault(name, []).append(value[start:start + length])
            for name, value in labels.items():
                label_batch.setdefault(name, []).append(value[start:start + length])
        obs = {name: torch.from_numpy(np.stack(value)).to(device)
               for name, value in obs_batch.items()}
        labels = {name: torch.from_numpy(np.stack(value)).to(device)
                  for name, value in label_batch.items()}
        return obs, labels


class StatefulBatcher:
    """Sequential TBPTT streams whose recurrent state spans complete games."""

    def __init__(self, source, size, length, rng, device):
        self.source, self.size, self.length = source, size, length
        self.rng, self.device = rng, device
        self.slots = [None] * size

    def _new_slot(self):
        dataset = (self.rng.choice(self.source.datasets)
                   if isinstance(self.source, ClusterBalancedSet) else self.source)
        eligible = [index for index, (obs, _) in enumerate(dataset.trajectories)
                    if len(obs["clock"]) >= self.length]
        return dataset, self.rng.choice(eligible), 0

    def batch(self):
        obs_batch, label_batch = {}, {}
        reset = np.zeros(self.size, dtype=np.bool_)
        for slot in range(self.size):
            current = self.slots[slot]
            if current is None:
                current, reset[slot] = self._new_slot(), True
            dataset, trajectory, start = current
            obs, labels = dataset.trajectories[trajectory]
            if start + self.length > len(obs["clock"]):
                dataset, trajectory, start = self._new_slot()
                obs, labels = dataset.trajectories[trajectory]
                reset[slot] = True
            for name, value in obs.items():
                obs_batch.setdefault(name, []).append(value[start:start + self.length])
            for name, value in labels.items():
                label_batch.setdefault(name, []).append(value[start:start + self.length])
            self.slots[slot] = (dataset, trajectory, start + self.length)
        observations = {name: torch.from_numpy(np.stack(value)).to(self.device)
                        for name, value in obs_batch.items()}
        labels = {name: torch.from_numpy(np.stack(value)).to(self.device)
                  for name, value in label_batch.items()}
        return observations, labels, torch.from_numpy(reset).to(self.device)


def _bit_mask(bits, width):
    shifts = torch.arange(width, device=bits.device, dtype=torch.int64)
    return ((bits.to(torch.int64).unsqueeze(-1) >> shifts) & 1).bool()


def _quantity_mask(maximum):
    classes = torch.arange(schema.N_QUANTITIES, device=maximum.device)
    return ((classes[None] >= 1) &
            (classes[None] <= torch.minimum(maximum[:, None], maximum.new_tensor(100)))) | (
                classes[None].eq(schema.ALL_AVAILABLE) & maximum[:, None].gt(100))


def _masked_cross_entropy(logits, targets, mask, weight=None, reduction="mean"):
    if not torch.all(mask.gather(-1, targets.unsqueeze(-1)).squeeze(-1)):
        raise RuntimeError("training target falls outside the shared legality mask")
    return nn.functional.cross_entropy(
        logits.masked_fill(~mask, -torch.inf), targets, weight=weight, reduction=reduction)


def _quantity_loss(logits, targets, mask, maximum, reduction="mean"):
    """Categorical exact count plus an ordinal expected-count auxiliary."""
    cross_entropy = _masked_cross_entropy(logits, targets, mask, reduction=reduction)
    probabilities = torch.softmax(logits.masked_fill(~mask, -torch.inf), dim=-1)
    values = torch.arange(schema.N_QUANTITIES, device=logits.device,
                          dtype=logits.dtype)[None].expand_as(logits).clone()
    values[:, schema.ALL_AVAILABLE] = maximum.to(logits.dtype)
    expected = (probabilities * values).sum(dim=-1) / maximum.clamp_min(1).to(logits.dtype)
    target_values = torch.where(targets.eq(schema.ALL_AVAILABLE), maximum, targets)
    ordinal = nn.functional.smooth_l1_loss(
        expected, target_values.to(logits.dtype) / maximum.clamp_min(1).to(logits.dtype),
        reduction=reduction)
    return cross_entropy + 0.2 * ordinal


def step_loss(model, output, labels, obs, time_index: int):
    active = obs["active_units"][:, time_index, 0].bool()
    unit_op = labels["unit_op"][:, time_index]
    unit_weights = output["unit_op_logits"].new_tensor(
        [0.25, 0.6, 0.6, 0.6, 0.6, 1.5, 1.2, 1.8, 2.0,
         1.2, 1.8, 1.8, 1.5, 2.0, 2.0, 1.8, 1.8, 1.5])
    op_mask = _bit_mask(labels["unit_op_bits"][:, time_index], schema.N_UNIT_OPS)
    operation_loss = _masked_cross_entropy(
        output["unit_op_logits"][active], unit_op[active], op_mask[active], unit_weights)

    batch_index, unit_index = active.nonzero(as_tuple=True)
    active_ops = unit_op[batch_index, unit_index]
    item_relevant = torch.zeros_like(active_ops, dtype=torch.bool)
    quantity_relevant = torch.zeros_like(active_ops, dtype=torch.bool)
    for operation in ITEM_UNIT_OPS:
        item_relevant |= active_ops.eq(int(operation))
    for operation in QUANTITY_UNIT_OPS:
        quantity_relevant |= active_ops.eq(int(operation))
    item_logits = output["unit_item_logits"][batch_index, unit_index, active_ops]
    item_targets = labels["unit_item"][:, time_index][batch_index, unit_index]
    item_mask = _bit_mask(labels["unit_item_bits"][:, time_index][batch_index, unit_index],
                          schema.N_ITEMS)
    item_loss = (_masked_cross_entropy(item_logits[item_relevant], item_targets[item_relevant],
                                       item_mask[item_relevant])
                 if item_relevant.any() else operation_loss.new_zeros(()))
    quantity_logits = output["unit_quantity_logits"][batch_index, unit_index]
    quantity_targets = labels["unit_quantity"][:, time_index][batch_index, unit_index]
    quantity_max = labels["unit_quantity_max"][:, time_index][batch_index, unit_index]
    qmask = _quantity_mask(quantity_max)
    quantity_loss = (_quantity_loss(
        quantity_logits[quantity_relevant], quantity_targets[quantity_relevant],
        qmask[quantity_relevant], quantity_max[quantity_relevant])
        if quantity_relevant.any() else operation_loss.new_zeros(()))

    market_op = labels["market_op"][:, time_index]
    market_item = labels["market_item"][:, time_index]
    market_quantity = labels["market_quantity"][:, time_index]
    market_valid = labels["market_valid"][:, time_index].bool()
    hidden = output["market_hidden"]
    op_logits = output["market_op_logits"]
    item_logits = output["market_item_logits"]
    quantity_logits = output["market_quantity_logits"]
    market_op_loss = operation_loss.new_zeros(())
    market_item_loss = operation_loss.new_zeros(())
    market_quantity_loss = operation_loss.new_zeros(())
    op_count = item_count = quantity_count = 0
    for slot in range(schema.MAX_ORDERS):
        valid = market_valid[:, slot]
        if valid.any():
            mask = _bit_mask(labels["market_op_bits"][:, time_index, slot],
                             schema.N_MARKET_OPS)
            market_weights = op_logits.new_tensor([1.0, 1.7, 1.7, 1.7, 1.7, 1.7, 1.7, 0.3])
            market_op_loss += _masked_cross_entropy(
                op_logits[valid], market_op[:, slot][valid], mask[valid],
                market_weights, reduction="sum")
            op_count += int(valid.sum())
        relevant = torch.zeros_like(valid)
        for operation in ITEM_MARKET_OPS:
            relevant |= market_op[:, slot].eq(int(operation))
        relevant &= valid
        if relevant.any():
            imask = _bit_mask(labels["market_item_bits"][:, time_index, slot],
                              schema.N_ITEMS)
            qmask = _quantity_mask(labels["market_quantity_max"][:, time_index, slot])
            market_item_loss += _masked_cross_entropy(
                item_logits[relevant], market_item[:, slot][relevant], imask[relevant],
                reduction="sum")
            market_quantity_loss += _quantity_loss(
                quantity_logits[relevant], market_quantity[:, slot][relevant], qmask[relevant],
                labels["market_quantity_max"][:, time_index, slot][relevant], reduction="sum")
            item_count += int(relevant.sum())
            quantity_count += int(relevant.sum())
        feedback_op = market_op[:, slot].clamp(0, schema.N_MARKET_OPS - 1)
        next_output = model.market_step(hidden, feedback_op, market_item[:, slot])
        hidden = next_output["hidden"]
        op_logits = next_output["op_logits"]
        item_logits = next_output["item_logits"]
        quantity_logits = next_output["quantity_logits"]

    market_op_loss /= max(op_count, 1)
    market_item_loss /= max(item_count, 1)
    market_quantity_loss /= max(quantity_count, 1)
    wdl_loss = nn.functional.cross_entropy(output["wdl_logits"], labels["wdl"][:, time_index])
    margin_loss = nn.functional.smooth_l1_loss(output["margin"], labels["margin"][:, time_index].float())
    opponent_flow_loss = nn.functional.smooth_l1_loss(
        output["opponent_flow"], labels["opponent_flow"][:, time_index].float())
    total = (operation_loss + 0.3 * item_loss + 0.1 * quantity_loss
             + 0.6 * market_op_loss + 0.25 * market_item_loss
             + 0.1 * market_quantity_loss + 0.2 * wdl_loss + 0.02 * margin_loss
             + 0.05 * opponent_flow_loss)
    return total


def step_metrics(model, output, labels, obs, time_index: int):
    active = obs["active_units"][:, time_index, 0].bool()
    target_op = labels["unit_op"][:, time_index]
    op_mask = _bit_mask(labels["unit_op_bits"][:, time_index], schema.N_UNIT_OPS)
    predicted_op = output["unit_op_logits"].masked_fill(~op_mask, -torch.inf).argmax(-1)
    unit_exact = predicted_op.eq(target_op)
    target_item = labels["unit_item"][:, time_index]
    target_quantity = labels["unit_quantity"][:, time_index]
    for operation in ITEM_UNIT_OPS:
        relevant = active & target_op.eq(int(operation))
        if relevant.any():
            item_mask = _bit_mask(labels["unit_item_bits"][:, time_index], schema.N_ITEMS)
            logits = output["unit_item_logits"][:, :, int(operation)]
            unit_exact[relevant] &= logits.masked_fill(~item_mask, -torch.inf).argmax(-1)[relevant].eq(
                target_item[relevant])
    for operation in QUANTITY_UNIT_OPS:
        relevant = active & target_op.eq(int(operation))
        if relevant.any():
            qmask = _quantity_mask(labels["unit_quantity_max"][:, time_index].reshape(-1)).reshape(
                *target_quantity.shape, schema.N_QUANTITIES)
            predicted = output["unit_quantity_logits"].masked_fill(~qmask, -torch.inf).argmax(-1)
            unit_exact[relevant] &= predicted[relevant].eq(target_quantity[relevant])

    market_exact = torch.ones_like(labels["market_valid"][:, time_index], dtype=torch.bool)
    hidden = output["market_hidden"]
    op_logits = output["market_op_logits"]
    item_logits = output["market_item_logits"]
    quantity_logits = output["market_quantity_logits"]
    market_op_correct = market_op_total = 0
    for slot in range(schema.MAX_ORDERS):
        valid = labels["market_valid"][:, time_index, slot].bool()
        target = labels["market_op"][:, time_index, slot]
        mask = _bit_mask(labels["market_op_bits"][:, time_index, slot], schema.N_MARKET_OPS)
        predicted = op_logits.masked_fill(~mask, -torch.inf).argmax(-1)
        market_exact[:, slot] &= predicted.eq(target)
        market_op_correct += int((predicted[valid] == target[valid]).sum())
        market_op_total += int(valid.sum())
        relevant = valid & target.ne(schema.MARKET_END) & target.ne(int(schema.MarketOp.HIRE)) & target.ne(
            int(schema.MarketOp.BUY_LAND))
        if relevant.any():
            imask = _bit_mask(labels["market_item_bits"][:, time_index, slot], schema.N_ITEMS)
            qmask = _quantity_mask(labels["market_quantity_max"][:, time_index, slot])
            market_exact[relevant, slot] &= item_logits.masked_fill(~imask, -torch.inf).argmax(-1)[
                relevant].eq(labels["market_item"][:, time_index, slot][relevant])
            market_exact[relevant, slot] &= quantity_logits.masked_fill(~qmask, -torch.inf).argmax(-1)[
                relevant].eq(labels["market_quantity"][:, time_index, slot][relevant])
        feedback_op = target.clamp(0, schema.N_MARKET_OPS - 1)
        next_output = model.market_step(
            hidden, feedback_op, labels["market_item"][:, time_index, slot])
        hidden = next_output["hidden"]
        op_logits, item_logits, quantity_logits = (next_output["op_logits"],
                                                    next_output["item_logits"],
                                                    next_output["quantity_logits"])
    joint = ((unit_exact | ~active).all(dim=1) &
             (market_exact | ~labels["market_valid"][:, time_index].bool()).all(dim=1))
    wdl_target = labels["wdl"][:, time_index]
    wdl_probabilities = torch.softmax(output["wdl_logits"], dim=-1)
    wdl_one_hot = nn.functional.one_hot(wdl_target, 3).to(wdl_probabilities.dtype)
    return {
        "unit_op_correct": int((predicted_op[active] == target_op[active]).sum()),
        "unit_total": int(active.sum()), "market_op_correct": market_op_correct,
        "market_total": market_op_total, "joint_correct": int(joint.sum()),
        "joint_total": int(joint.numel()),
        "wdl_correct": int(output["wdl_logits"].argmax(-1).eq(wdl_target).sum()),
        "wdl_brier_sum": float(((wdl_probabilities - wdl_one_hot) ** 2).sum()),
        "wdl_total": int(joint.numel()),
    }


def train_steps(model, dataset, steps, sequence_length, batch_size, optimizer,
                device, rng, log_every, burn_in=0, start_update=0,
                target_updates=None, base_learning_rate=None, warmup_updates=0,
                checkpoint_every=0, checkpoint_callback=None,
                stateful_recurrence=True):
    model.train()
    history = []
    if steps == 0:
        print(f"[optimize] already at requested update {start_update}", flush=True)
        return history
    progress = Progress("optimize", steps)
    stream = (StatefulBatcher(dataset, batch_size, sequence_length, rng, device)
              if stateful_recurrence else None)
    stream_state = None
    for relative_update in range(1, steps + 1):
        update = start_update + relative_update
        if base_learning_rate is not None and target_updates:
            if warmup_updates and update <= warmup_updates:
                scale = update / max(1, warmup_updates)
            else:
                progress_fraction = ((update - warmup_updates) /
                                     max(1, target_updates - warmup_updates))
                scale = 0.05 + 0.95 * 0.5 * (1.0 + math.cos(
                    math.pi * min(1.0, max(0.0, progress_fraction))))
            for group in optimizer.param_groups:
                group["lr"] = base_learning_rate * scale
        # Three updates preserve recurrent state across consecutive chunks.  A
        # fourth uses the stratified sampler so rare strategic/endgame choices
        # remain visible without breaking the continuing streams.
        use_stream = stream is not None and relative_update % 4 != 0
        if use_stream:
            obs, labels, reset = stream.batch()
            total_length, loss_start = sequence_length, 0
            if stream_state is None:
                state = model.initial_state(batch_size, device=device)
            else:
                keep = (~reset).to(stream_state.hourly.dtype)[:, None]
                state = type(stream_state)(stream_state.hourly * keep,
                                           stream_state.daily * keep)
        else:
            total_length = burn_in + sequence_length
            loss_start = burn_in
            obs, labels = dataset.batch(batch_size, total_length, rng, device)
            state = None
        loss = torch.zeros((), device=device)
        if loss_start:
            with torch.no_grad(), torch.autocast(
                    device_type=device.type, dtype=torch.bfloat16,
                    enabled=device.type == "cuda"):
                for time_index in range(loss_start):
                    current = {name: value[:, time_index] for name, value in obs.items()}
                    state = model(current, state)["state"]
        with torch.autocast(device_type=device.type, dtype=torch.bfloat16,
                            enabled=device.type == "cuda"):
            for time_index in range(loss_start, total_length):
                current = {name: value[:, time_index] for name, value in obs.items()}
                output = model(current, state)
                state = output["state"]
                loss = loss + step_loss(model, output, labels, obs, time_index)
            loss /= sequence_length
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        optimizer.step()
        if use_stream:
            stream_state = type(state)(state.hourly.detach(), state.daily.detach())
        record = (relative_update == 1 or update % log_every == 0
                  or relative_update == steps)
        if record:
            value = float(loss.detach())
            history.append({"update": update, "loss": value})
        detail = f"update={update} loss={value:.4f}" if record else None
        progress.update(relative_update, detail=detail, force=record)
        if checkpoint_callback and checkpoint_every and update % checkpoint_every == 0:
            checkpoint_callback(update, history)
            model.train()
    return history


@torch.no_grad()
def evaluate(model, dataset, sequence_length, batch_size, batches, device, seed,
             burn_in=0):
    """Estimate teacher-forced held-out loss with deterministic window sampling."""
    model.eval()
    rng = random.Random(seed)
    losses = []
    totals = {name: 0 for name in ("unit_op_correct", "unit_total", "market_op_correct",
                                   "market_total", "joint_correct", "joint_total",
                                   "wdl_correct", "wdl_brier_sum", "wdl_total")}
    progress = Progress("validate", batches)
    for batch_index in range(1, batches + 1):
        total_length = burn_in + sequence_length
        obs, labels = dataset.batch(batch_size, total_length, rng, device)
        state = None
        loss = torch.zeros((), device=device)
        with torch.autocast(device_type=device.type, dtype=torch.bfloat16,
                            enabled=device.type == "cuda"):
            for time_index in range(total_length):
                current = {name: value[:, time_index] for name, value in obs.items()}
                output = model(current, state)
                state = output["state"]
                if time_index >= burn_in:
                    loss += step_loss(model, output, labels, obs, time_index)
                    for name, value in step_metrics(model, output, labels, obs, time_index).items():
                        totals[name] += value
            loss /= sequence_length
        losses.append(float(loss))
        progress.update(batch_index, detail=f"loss={losses[-1]:.4f}")
    return {
        "loss": float(np.mean(losses)), "batches": batches,
        "sampled_sequences": batches * batch_size,
        "legal_unit_operation_accuracy": totals["unit_op_correct"] / max(1, totals["unit_total"]),
        "legal_market_operation_accuracy": totals["market_op_correct"] / max(1, totals["market_total"]),
        "exact_joint_action_rate": totals["joint_correct"] / max(1, totals["joint_total"]),
        "wdl_accuracy": totals["wdl_correct"] / max(1, totals["wdl_total"]),
        "wdl_brier_score": totals["wdl_brier_sum"] / max(1, totals["wdl_total"]),
    }


def split_clusters(selected, validation_fraction, seed):
    """Partition replay files globally and keep validation in every lineage."""
    paths = sorted({path for members in selected.values() for path, _ in members})
    scored = sorted(paths, key=lambda path: hashlib.sha256(
        f"{seed}:{path}".encode()).digest())
    count = (max(1, round(len(paths) * validation_fraction))
             if validation_fraction > 0 and len(paths) > 1 else 0)
    validation_paths = set(scored[:count])
    if validation_fraction:
        for members in selected.values():
            member_paths = sorted({path for path, _ in members})
            if len(member_paths) > 1 and not validation_paths.intersection(member_paths):
                validation_paths.add(min(member_paths, key=lambda path: hashlib.sha256(
                    f"validation:{seed}:{path}".encode()).digest()))
    train, validation = {}, {}
    for name, members in selected.items():
        train[name] = [row for row in members if row[0] not in validation_paths]
        validation[name] = [row for row in members if row[0] in validation_paths]
        if not train[name]:
            raise ValueError(
                f"global replay split left cluster {name!r} without training data")
    return train, validation


def cluster_statistics(metadata):
    result = {}
    for row in metadata:
        stats = result.setdefault(row["cluster"], {
            "team": row["team"], "submission_id": row["submission_id"],
            "trajectories": 0, "wins": 0, "draws": 0, "losses": 0,
            "margin_sum": 0.0,
        })
        stats["trajectories"] += 1
        delta = float(row["reward"] - row["opponent_reward"])
        stats["margin_sum"] += delta
        stats["wins" if delta > 0 else "losses" if delta < 0 else "draws"] += 1
    for stats in result.values():
        count = stats["trajectories"]
        rate = (stats["wins"] + 0.5 * stats["draws"]) / count
        # Wilson lower bound prevents tiny lucky samples from outranking robust
        # submissions; margin is only a bounded tie-breaker.
        z = 1.96
        denominator = 1.0 + z * z / count
        centre = rate + z * z / (2 * count)
        spread = z * np.sqrt((rate * (1 - rate) + z * z / (4 * count)) / count)
        stats["win_rate"] = rate
        stats["average_margin"] = stats["margin_sum"] / count
        stats["selection_score"] = ((centre - spread) / denominator
                                    + 0.02 * np.tanh(stats["average_margin"] / 3000.0))
    return result


def select_diverse_lineages(groups, metadata, minimum, maximum, min_win_rate):
    stats = cluster_statistics(metadata)
    eligible = [(name, members) for name, members in groups.items()
                if len(members) >= minimum and stats.get(name, {}).get("win_rate", 0) >= min_win_rate]
    eligible.sort(key=lambda row: (-stats[row[0]]["selection_score"],
                                  -len(row[1]), row[0]))
    selected, used_teams = [], set()
    for row in eligible:
        team = stats[row[0]]["team"]
        if team not in used_teams:
            selected.append(row)
            used_teams.add(team)
        if len(selected) == maximum:
            break
    if len(selected) < maximum:
        selected_names = {name for name, _ in selected}
        selected.extend(row for row in eligible if row[0] not in selected_names)
        selected = selected[:maximum]
    return selected, stats


def save_checkpoint(path, model, optimizer, config, cluster, members, history,
                    seed, completed_updates, data_rng, recipe):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    torch.save({"format": 2, "model": model.state_dict(), "model_config": asdict(config),
                "optimizer": optimizer.state_dict(),
                "cluster": cluster, "members": [(str(p), s) for p, s in members],
                "recipe": recipe,
                "history": history, "seed": seed,
                "completed_updates": completed_updates,
                "data_rng_state": data_rng.getstate(),
                "torch_rng_state": torch.get_rng_state(),
                "cuda_rng_state": torch.cuda.get_rng_state_all()
                if torch.cuda.is_available() else None}, temporary)
    temporary.replace(path)


def restore_checkpoint(path, model, optimizer, data_rng, device, config,
                       cluster, members, recipe):
    checkpoint = torch.load(path, map_location=device, weights_only=False)
    expected_members = [(str(path), seat) for path, seat in members]
    if checkpoint["model_config"] != asdict(config):
        raise ValueError(f"model configuration differs from checkpoint {path}")
    if checkpoint["cluster"] != cluster or checkpoint["members"] != expected_members:
        raise ValueError(f"data lineage differs from checkpoint {path}")
    if checkpoint.get("format") != 2 or checkpoint.get("recipe") != recipe:
        raise ValueError(f"training recipe differs from checkpoint {path}")
    model.load_state_dict(checkpoint["model"])
    optimizer.load_state_dict(checkpoint["optimizer"])
    data_rng.setstate(checkpoint["data_rng_state"])
    torch.set_rng_state(checkpoint["torch_rng_state"].cpu())
    if device.type == "cuda" and checkpoint.get("cuda_rng_state") is not None:
        torch.cuda.set_rng_state_all(
            [state.cpu() for state in checkpoint["cuda_rng_state"]])
    return int(checkpoint["completed_updates"]), list(checkpoint["history"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--replays", type=Path, default=Path("corpus/data_all"))
    parser.add_argument("--output", type=Path,
                        default=Path("reinforcement_learning/artifacts/seeds"))
    parser.add_argument("--cluster-manifest", type=Path)
    parser.add_argument("--team", action="append", help="exact team label; repeatable")
    parser.add_argument("--limit-replays", type=int)
    parser.add_argument("--max-seeds", type=int, default=8)
    parser.add_argument("--require-seeds", type=int, default=0,
                        help="fail before loading data unless at least this many seeds qualify")
    parser.add_argument("--min-trajectories", type=int, default=20)
    parser.add_argument("--min-win-rate", type=float, default=0.50,
                        help="minimum replay W/D/L score for an automatically selected seed")
    parser.add_argument("--max-trajectories-per-seed", type=int, default=128)
    parser.add_argument("--behavior-version-clustering", action=argparse.BooleanOptionalAction,
                        default=True, help="split team-only histories by replay action signature")
    parser.add_argument("--validation-fraction", type=float, default=0.15)
    parser.add_argument("--stride", type=int, default=1,
                        help="debug subsampling; production recurrent training uses 1")
    parser.add_argument("--sequence-length", type=int, default=16)
    parser.add_argument("--burn-in", type=int, default=0,
                        help="context steps before each loss-bearing recurrent sequence")
    parser.add_argument("--stateful-recurrence", action=argparse.BooleanOptionalAction,
                        default=True, help="carry detached memory across sequential replay chunks")
    parser.add_argument("--global-steps", type=int, default=20000)
    parser.add_argument("--finetune-steps", type=int, default=5000)
    parser.add_argument("--batch-size", type=int, default=8)
    parser.add_argument("--device", choices=("cuda", "cpu", "auto"), default="cuda",
                        help="production defaults to CUDA and fails rather than silently using CPU")
    parser.add_argument("--learning-rate", type=float, default=2e-4)
    parser.add_argument("--finetune-learning-rate", type=float, default=5e-5)
    parser.add_argument("--seed", type=int, default=20260909)
    parser.add_argument("--log-every", type=int, default=100)
    parser.add_argument("--checkpoint-every", type=int, default=250)
    parser.add_argument("--warmup-updates", type=int, default=250)
    parser.add_argument("--validation-batches", type=int, default=20)
    parser.add_argument("--validation-every", type=int, default=500,
                        help="score and retain the best seed every N updates; 0 disables interim scoring")
    parser.add_argument("--resume", action="store_true",
                        help="resume optimizer and RNG state from existing checkpoints")
    args = parser.parse_args()

    if not 0 <= args.validation_fraction < 0.5:
        raise SystemExit("--validation-fraction must be in [0, 0.5)")
    if args.sequence_length < 1 or args.batch_size < 1 or args.stride < 1:
        raise SystemExit("--sequence-length, --batch-size, and --stride must be positive")
    if args.burn_in < 0:
        raise SystemExit("--burn-in cannot be negative")
    if args.global_steps < 0 or args.finetune_steps < 0 or args.validation_batches < 0:
        raise SystemExit("update counts and --validation-batches cannot be negative")
    if args.checkpoint_every < 1 or args.warmup_updates < 0:
        raise SystemExit("--checkpoint-every must be positive and --warmup-updates non-negative")
    if args.device == "cuda" and not torch.cuda.is_available():
        raise SystemExit("CUDA was requested but is unavailable; fix GPU visibility or use --device cpu for a smoke test")
    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available()
                          else "cpu" if args.device == "auto" else args.device)
    recipe_base = {
        "version": 2, "stride": args.stride, "burn_in": args.burn_in,
        "sequence_length": args.sequence_length, "batch_size": args.batch_size,
        "weight_decay": 1e-4, "label": "solo-market/joint-unit-canonical",
        "legality_masks": "structured-codec-v2-inventory-order",
        "sampling": "stateful-3-to-stratified-1-v1" if args.stateful_recurrence else "stratified-windows-v1",
        "warmup_updates": args.warmup_updates,
    }
    paths = sorted(args.replays.glob("*.kagz"))
    if args.limit_replays:
        random.Random(args.seed).shuffle(paths)
        paths = paths[:args.limit_replays]
    groups, metadata = discover(
        paths, set(args.team) if args.team else None,
        compute_fingerprints=args.behavior_version_clustering and not args.cluster_manifest)
    if args.behavior_version_clustering and not args.cluster_manifest:
        groups = split_behavior_versions(groups, metadata, args.min_trajectories, args.seed)
    groups = apply_manifest(groups, args.cluster_manifest)
    if args.cluster_manifest:
        assignments = {(str(path), seat): cluster
                       for cluster, members in groups.items() for path, seat in members}
        metadata = [{**row, "cluster": assignments[(row["path"], row["seat"])]}
                    for row in metadata if (row["path"], row["seat"]) in assignments]
    eligible, statistics = select_diverse_lineages(
        groups, metadata, args.min_trajectories, args.max_seeds,
        args.min_win_rate if not args.cluster_manifest and not args.team else 0.0)
    if not eligible:
        raise SystemExit("no clusters satisfy --min-trajectories")
    if len(eligible) < args.require_seeds:
        raise SystemExit(f"only {len(eligible)} lineages qualified; required {args.require_seeds}")

    selected = {}
    print("selected strong/diverse replay lineages", flush=True)
    for index, (name, members) in enumerate(eligible):
        members = sorted(members, key=lambda row: (str(row[0]), row[1]))
        random.Random(args.seed + index).shuffle(members)
        selected[name] = members[:args.max_trajectories_per_seed]
        stats = statistics[name]
        print(f"  {name!r}: n={len(members)} win_rate={stats['win_rate']:.3f} "
              f"average_margin={stats['average_margin']:+.1f} "
              f"selection_score={stats['selection_score']:.3f}", flush=True)
    train_members, validation_members = split_clusters(
        selected, args.validation_fraction, args.seed)
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = {
        "seed": args.seed,
        "source": str(args.replays),
        "grouping": "reviewed manifest" if args.cluster_manifest else "team plus submission id",
        "selection_statistics": {name: statistics.get(name) for name in selected},
        "clusters": {name: [{"path": str(p), "seat": seat} for p, seat in members]
                     for name, members in selected.items()},
        "splits": {
            name: {
                "train": [{"path": str(p), "seat": seat}
                          for p, seat in train_members[name]],
                "validation": [{"path": str(p), "seat": seat}
                               for p, seat in validation_members[name]],
            }
            for name in selected
        },
        "discovered_trajectories": len(metadata),
    }
    manifest_path = args.output / "manifest.json"
    serialized_manifest = json.dumps(manifest, indent=2) + "\n"
    if args.resume and manifest_path.exists() and manifest_path.read_text() != serialized_manifest:
        raise RuntimeError("resume manifest differs; use a new output directory")
    manifest_path.write_text(serialized_manifest)

    print("loading canonical trajectory sequences", flush=True)
    train_sets = {}
    all_train_members = []
    for name in selected:
        train = train_members[name]
        validation = validation_members[name]
        all_train_members.extend(train)
        dataset = TrajectorySet(train, args.stride, f"load train {name}")
        dataset.build_windows(args.burn_in + args.sequence_length)
        train_sets[name] = dataset
        print(f"  {name!r}: {len(train)} train / {len(validation)} validation trajectories")

    # The global warm start samples lineages uniformly and reuses the already
    # decoded trajectory sets. Independent descendants are copied afterward.
    global_set = ClusterBalancedSet(train_sets.values())
    torch.manual_seed(args.seed)
    if device.type == "cuda":
        torch.cuda.manual_seed_all(args.seed)
    config = ModelConfig()
    model = KaggricultureActor(config).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.learning_rate,
                                  weight_decay=1e-4)
    global_rng = random.Random(args.seed)
    global_checkpoint = args.output / "global.pt"
    global_recipe = {**recipe_base, "stage": "global",
                     "target_updates": args.global_steps,
                     "learning_rate": args.learning_rate}
    global_completed, global_history = 0, []
    if args.resume and global_checkpoint.exists():
        global_completed, global_history = restore_checkpoint(
            global_checkpoint, model, optimizer, global_rng, device, config,
            "global", all_train_members, global_recipe)
        print(f"resumed global checkpoint at update {global_completed}", flush=True)
    remaining = max(0, args.global_steps - global_completed)
    started = time.time()
    print(f"global warm start on {device}", flush=True)
    prior_global_history = list(global_history)

    def save_global(update, partial_history):
        save_checkpoint(global_checkpoint, model, optimizer, config, "global",
                        all_train_members, prior_global_history + partial_history,
                        args.seed, update, global_rng, global_recipe)

    global_history += train_steps(
        model, global_set, remaining, args.sequence_length, args.batch_size,
        optimizer, device, global_rng, args.log_every, args.burn_in,
        global_completed, args.global_steps, args.learning_rate,
        args.warmup_updates, args.checkpoint_every, save_global,
        args.stateful_recurrence)
    save_checkpoint(global_checkpoint, model, optimizer, config, "global",
                    all_train_members, global_history, args.seed,
                    global_completed + remaining, global_rng, global_recipe)
    global_state = copy.deepcopy(model.state_dict())

    reports = {}
    for index, (name, members) in enumerate(selected.items()):
        print(f"fine-tuning seed {name!r}", flush=True)
        seed_model = KaggricultureActor(config).to(device)
        seed_model.load_state_dict(global_state)
        optimizer = torch.optim.AdamW(seed_model.parameters(), lr=args.finetune_learning_rate,
                                      weight_decay=1e-4)
        directory = args.output / slug(name)
        checkpoint = directory / "model.pt"
        best_checkpoint = directory / "best.pt"
        validation_set = None
        if validation_members[name] and args.validation_batches:
            validation_set = TrajectorySet(
                validation_members[name], args.stride, f"load validation {name}")
            validation_set.build_windows(args.burn_in + args.sequence_length)
        seed_rng = random.Random(args.seed + 1000 + index)
        completed, history = 0, []
        seed_recipe = {**recipe_base, "stage": "finetune",
                       "target_updates": args.finetune_steps,
                       "learning_rate": args.finetune_learning_rate,
                       "parent_updates": args.global_steps}
        if args.resume and checkpoint.exists():
            completed, history = restore_checkpoint(
                checkpoint, seed_model, optimizer, seed_rng, device, config,
                name, members, seed_recipe)
            print(f"  resumed at update {completed}", flush=True)
        remaining = max(0, args.finetune_steps - completed)
        prior_history = list(history)
        best = {"exact_joint_action_rate": -1.0, "loss": float("inf"),
                "update": None, "metrics": None}

        def save_seed(update, partial_history):
            save_checkpoint(checkpoint, seed_model, optimizer, config, name,
                            members, prior_history + partial_history,
                            args.seed + 1000 + index, update, seed_rng, seed_recipe)
            if (validation_set is not None and args.validation_every and
                    update % args.validation_every == 0):
                print(f"  validation checkpoint update={update}", flush=True)
                metrics = evaluate(
                    seed_model, validation_set, args.sequence_length,
                    args.batch_size, args.validation_batches, device,
                    args.seed + 2000 + index, args.burn_in)
                score = (metrics["exact_joint_action_rate"], -metrics["loss"])
                previous = (best["exact_joint_action_rate"], -best["loss"])
                if score > previous:
                    best.update({"exact_joint_action_rate": score[0],
                                 "loss": metrics["loss"], "update": update,
                                 "metrics": metrics})
                    save_checkpoint(best_checkpoint, seed_model, optimizer, config,
                                    name, members, prior_history + partial_history,
                                    args.seed + 1000 + index, update, seed_rng,
                                    seed_recipe)

        history += train_steps(
            seed_model, train_sets[name], remaining, args.sequence_length,
            args.batch_size, optimizer, device, seed_rng, args.log_every,
            args.burn_in, completed, args.finetune_steps,
            args.finetune_learning_rate, args.warmup_updates,
            args.checkpoint_every, save_seed, args.stateful_recurrence)
        save_checkpoint(checkpoint, seed_model, optimizer, config, name, members,
                        history, args.seed + 1000 + index,
                        completed + remaining, seed_rng, seed_recipe)
        validation = None
        if validation_set is not None:
            print("  scoring replay-disjoint validation split", flush=True)
            validation = evaluate(seed_model, validation_set, args.sequence_length,
                                  args.batch_size, args.validation_batches, device,
                                  args.seed + 2000 + index, args.burn_in)
            final_score = (validation["exact_joint_action_rate"], -validation["loss"])
            if final_score > (best["exact_joint_action_rate"], -best["loss"]):
                best.update({"exact_joint_action_rate": final_score[0],
                             "loss": validation["loss"],
                             "update": completed + remaining, "metrics": validation})
                save_checkpoint(best_checkpoint, seed_model, optimizer, config,
                                name, members, history, args.seed + 1000 + index,
                                completed + remaining, seed_rng, seed_recipe)
        selected_checkpoint = best_checkpoint if best_checkpoint.exists() else checkpoint
        report = {
            "cluster": name,
            "checkpoint": str(selected_checkpoint),
            "final_checkpoint": str(checkpoint),
            "best_validation": best if best["update"] is not None else None,
            "train_trajectories": len(train_sets[name].trajectories),
            "validation_members": [(str(p), seat)
                                   for p, seat in validation_members[name]],
            "last_training_loss": history[-1]["loss"] if history else None,
            "validation": validation,
            "completed_updates": completed + remaining,
        }
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        reports[name] = report

    summary = {
        "purpose": "behavior-clone initialization seeds; closed-loop promotion pending",
        "device": str(device), "model_config": asdict(config),
        "clusters": reports, "elapsed_seconds": time.time() - started,
        "arguments": {key: str(value) if isinstance(value, Path) else value
                      for key, value in vars(args).items()},
    }
    (args.output / "training_report.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
