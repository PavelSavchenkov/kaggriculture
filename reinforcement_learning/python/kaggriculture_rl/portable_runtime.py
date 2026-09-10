"""NumPy-only inference runtime for exported KaggricultureActor submissions.

This intentionally implements only the layers used by ``model.py``.  Submission
agents can therefore run in the stock LocalLB image, which has NumPy but not
PyTorch, while the exporter can test every output against the training model.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import numpy as np


BOARD = 10
MAX_UNITS = 40
N_ITEMS = 12
N_PRODUCTS = 9
N_CROPS = 5
N_SHOPS = 8
N_UNIT_OPS = 18
N_MARKET_OPS = 8
N_QUANTITIES = 102
SHOP_NAMES = (
    "BAKERY", "BRUNCH_SPOT", "FARMERS_MARKET", "ICE_CREAM_SHOP",
    "PET_CAFE", "PIZZA_SHOP", "SMOOTHIE_SHOP", "YARN_STORE",
)
ITEM_NAMES = (
    "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG",
    "MILK", "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP",
)
KIND_IDS = {"LOCKED": 1, "WEED": 2, "COOP": 3, "PASTURE": 4, "PLANT": 5}


def _get(value: Any, key: str, default: Any = None) -> Any:
    if isinstance(value, dict):
        return value.get(key, default)
    return getattr(value, key, default)


def _linear(x: np.ndarray, weight: np.ndarray, bias: np.ndarray) -> np.ndarray:
    return x @ weight.T + bias


def _sigmoid(x: np.ndarray) -> np.ndarray:
    return 1.0 / (1.0 + np.exp(-np.clip(x, -40.0, 40.0)))


def _silu(x: np.ndarray) -> np.ndarray:
    return x * _sigmoid(x)


def _symlog(x: np.ndarray) -> np.ndarray:
    return np.sign(x) * np.log1p(np.abs(x))


def _layer_norm(x: np.ndarray, weight: np.ndarray, bias: np.ndarray) -> np.ndarray:
    mean = x.mean(axis=-1, keepdims=True)
    variance = ((x - mean) ** 2).mean(axis=-1, keepdims=True)
    return (x - mean) / np.sqrt(variance + 1e-5) * weight + bias


def _group_norm(x: np.ndarray, weight: np.ndarray, bias: np.ndarray,
                groups: int = 8) -> np.ndarray:
    n, channels, height, width = x.shape
    grouped = x.reshape(n, groups, channels // groups, height, width)
    mean = grouped.mean(axis=(2, 3, 4), keepdims=True)
    variance = ((grouped - mean) ** 2).mean(axis=(2, 3, 4), keepdims=True)
    normalized = ((grouped - mean) / np.sqrt(variance + 1e-5)).reshape(x.shape)
    return normalized * weight[None, :, None, None] + bias[None, :, None, None]


def _conv2d(x: np.ndarray, weight: np.ndarray, bias: np.ndarray,
            padding: int = 0) -> np.ndarray:
    kh, kw = weight.shape[2:]
    if padding:
        x = np.pad(x, ((0, 0), (0, 0), (padding, padding), (padding, padding)))
    windows = np.lib.stride_tricks.sliding_window_view(x, (kh, kw), axis=(2, 3))
    result = np.einsum("nchwkl,ockl->nohw", windows, weight, optimize=True)
    return result + bias[None, :, None, None]


def _attention(q: np.ndarray, kv: np.ndarray, weights: dict[str, np.ndarray],
               prefix: str, heads: int, padding_mask: np.ndarray | None = None) -> np.ndarray:
    projection = weights[f"{prefix}.in_proj_weight"]
    projection_bias = weights[f"{prefix}.in_proj_bias"]
    width = q.shape[-1]
    qv = _linear(q, projection[:width], projection_bias[:width])
    kv_key = _linear(kv, projection[width:2 * width], projection_bias[width:2 * width])
    kv_value = _linear(kv, projection[2 * width:], projection_bias[2 * width:])
    depth = width // heads

    def split(value: np.ndarray) -> np.ndarray:
        return value.reshape(value.shape[0], value.shape[1], heads, depth).transpose(0, 2, 1, 3)

    qh, kh, vh = split(qv), split(kv_key), split(kv_value)
    scores = (qh @ kh.swapaxes(-1, -2)) / np.sqrt(float(depth))
    if padding_mask is not None:
        scores = np.where(padding_mask[:, None, None, :], -np.inf, scores)
    scores -= scores.max(axis=-1, keepdims=True)
    probabilities = np.exp(scores)
    probabilities /= probabilities.sum(axis=-1, keepdims=True)
    attended = probabilities @ vh
    attended = attended.transpose(0, 2, 1, 3).reshape(q.shape[0], q.shape[1], width)
    return _linear(attended, weights[f"{prefix}.out_proj.weight"],
                   weights[f"{prefix}.out_proj.bias"])


def _gru_cell(x: np.ndarray, hidden: np.ndarray,
              weights: dict[str, np.ndarray], prefix: str) -> np.ndarray:
    gi = _linear(x, weights[f"{prefix}.weight_ih"], weights[f"{prefix}.bias_ih"])
    gh = _linear(hidden, weights[f"{prefix}.weight_hh"], weights[f"{prefix}.bias_hh"])
    ir, iz, inn = np.split(gi, 3, axis=-1)
    hr, hz, hn = np.split(gh, 3, axis=-1)
    reset = _sigmoid(ir + hr)
    update = _sigmoid(iz + hz)
    candidate = np.tanh(inn + reset * hn)
    return (1.0 - update) * candidate + update * hidden


def encode_live_observation(observation: Any) -> dict[str, np.ndarray]:
    """Reproduce the actor-safe C++ observation tensors from a live observation."""
    player = int(_get(observation, "player", 0))
    farms = _get(observation, "farms", [])
    if player not in (0, 1) or len(farms) != 2:
        raise ValueError("expected a two-player Kaggriculture observation")
    day = int(_get(observation, "day", 0))
    hour = int(_get(observation, "hour", 0))
    step = day * 24 + hour
    tiles = np.zeros((1, 2, BOARD, BOARD, 13), dtype=np.int32)
    farm_features = np.zeros((1, 2, 4), dtype=np.float32)
    positions = np.full((1, 2, MAX_UNITS, 2), -1, dtype=np.int16)
    active = np.zeros((1, 2, MAX_UNITS), dtype=np.uint8)
    item_index = {name: index for index, name in enumerate(ITEM_NAMES)}
    for relative, seat in enumerate((player, 1 - player)):
        farm = farms[seat]
        hands = list(_get(farm, "hands", []) or [])
        unit_positions = [_get(farm, "farmer", [0, 0]), *hands]
        farm_features[0, relative] = (
            float(_get(farm, "money", 0)), len(unit_positions),
            len(_get(farm, "unlocked_quadrants", []) or []),
            int(_get(farm, "hires_today", 0)),
        )
        for unit, position in enumerate(unit_positions[:MAX_UNITS]):
            positions[0, relative, unit] = (int(position[0]), int(position[1]))
            active[0, relative, unit] = 1
        board = _get(farm, "tiles", [])
        for y in range(BOARD):
            for x in range(BOARD):
                tile = board[y][x]
                if tile is None:
                    tiles[0, relative, y, x, 11] = -1
                    tiles[0, relative, y, x, 12] = -1
                    continue
                if tile == "LOCKED":
                    tiles[0, relative, y, x, 0] = 1
                    tiles[0, relative, y, x, 11] = -1
                    tiles[0, relative, y, x, 12] = -1
                    continue
                kind = str(_get(tile, "kind", "WEED"))
                tiles[0, relative, y, x, 0] = KIND_IDS.get(kind, 2)
                crop = _get(tile, "crop", None)
                animal = _get(tile, "animal", None)
                tiles[0, relative, y, x, 1] = item_index.get(
                    str(animal if animal is not None else crop), 0)
                tiles[0, relative, y, x, 2] = int(animal is not None)
                tiles[0, relative, y, x, 3] = int(bool(_get(tile, "watered_today", False)))
                tiles[0, relative, y, x, 4] = int(bool(_get(tile, "fed_today", False)))
                tiles[0, relative, y, x, 5] = int(bool(_get(tile, "cared_today", False)))
                tiles[0, relative, y, x, 6] = int(bool(_get(tile, "fertilizer_available", False)))
                dry = _get(tile, "consecutive_unwatered",
                           _get(tile, "consecutive_unfed", 0))
                tiles[0, relative, y, x, 7] = int(dry)
                tiles[0, relative, y, x, 8] = int(_get(tile, "yield_units", 0))
                tiles[0, relative, y, x, 9] = int(_get(tile, "pending_care_bonus", 0))
                tiles[0, relative, y, x, 10] = int(
                    _get(tile, "planted_day", _get(tile, "placed_day", 0)))
                tiles[0, relative, y, x, 11] = int(_get(tile, "max_lifespan_step", -1))
                tiles[0, relative, y, x, 12] = int(_get(tile, "fertilized_until_day", -1))

    private = _get(observation, "private", {}) or {}
    shed_map = _get(private, "shed", {}) or {}
    seed_map = _get(private, "seeds", {}) or {}
    inventories = list(_get(private, "inventories", []) or [])
    own_shed = np.array([[int(_get(shed_map, item, 0)) for item in ITEM_NAMES]], dtype=np.int32)
    own_seeds = np.array([[int(_get(seed_map, item, 0)) for item in ITEM_NAMES[:N_CROPS]]],
                         dtype=np.int32)
    own_inventory = np.zeros((1, MAX_UNITS, N_ITEMS), dtype=np.int32)
    own_inventory_order = np.zeros((1, MAX_UNITS, N_ITEMS), dtype=np.uint8)
    for unit, inventory in enumerate(inventories[:MAX_UNITS]):
        own_inventory[0, unit] = [int(_get(inventory, item, 0)) for item in ITEM_NAMES]
        rank = 1
        for name, quantity in inventory.items():
            if name in item_index and int(quantity) > 0:
                own_inventory_order[0, unit, item_index[name]] = rank
                rank += 1
    market_map = _get(observation, "market", {}) or {}
    market_inventory = _get(market_map, "inventory", {}) or {}
    market_prices = _get(market_map, "prices", {}) or {}
    market = np.array([[[int(_get(market_inventory, item, 0)),
                         int(_get(market_prices, item, 0))]
                        for item in ITEM_NAMES[:N_PRODUCTS]]], dtype=np.int32)
    unlocked_shops = list(_get(_get(observation, "town", {}) or {},
                               "unlocked_shops", []) or [])
    shops = np.array([[unlocked_shops.count(name) for name in SHOP_NAMES]], dtype=np.int32)
    clock = np.array([[step, day, hour, player]], dtype=np.int32)
    return {"tiles": tiles, "farm": farm_features, "positions": positions,
            "active_units": active, "own_shed": own_shed,
            "own_seeds": own_seeds, "own_inventory": own_inventory,
            "own_inventory_order": own_inventory_order,
            "market": market, "shops": shops, "clock": clock}


class PortableActor:
    def __init__(self, weights: dict[str, np.ndarray], config: dict[str, int]):
        self.w = {key: np.asarray(value, dtype=np.float32)
                  for key, value in weights.items() if not key.startswith("__")}
        self.config = config

    @classmethod
    def load(cls, path: str | Path) -> "PortableActor":
        with np.load(path, allow_pickle=False) as archive:
            config = json.loads(str(archive["__config_json"].item()))
            weights = {key: archive[key] for key in archive.files if not key.startswith("__")}
        return cls(weights, config)

    def initial_state(self) -> tuple[np.ndarray, np.ndarray]:
        width = int(self.config["memory_width"])
        return np.zeros((1, width), np.float32), np.zeros((1, width), np.float32)

    def _mlp(self, x: np.ndarray, prefix: str) -> np.ndarray:
        x = _linear(x, self.w[f"{prefix}.0.weight"], self.w[f"{prefix}.0.bias"])
        x = _silu(x)
        return _linear(x, self.w[f"{prefix}.2.weight"], self.w[f"{prefix}.2.bias"])

    def _board_tokens(self, tiles: np.ndarray) -> np.ndarray:
        kind = np.eye(6, dtype=np.float32)[np.clip(tiles[..., 0], 0, 5)]
        item = np.eye(N_ITEMS, dtype=np.float32)[np.clip(tiles[..., 1], 0, N_ITEMS - 1)]
        item *= ((tiles[..., 0] == 5) | (tiles[..., 2] != 0))[..., None]
        x = np.concatenate((kind, item, tiles[..., 2:7].astype(np.float32),
                            _symlog(tiles[..., 7:13].astype(np.float32))), axis=-1)
        x = x.transpose(0, 1, 4, 2, 3).reshape(2, -1, BOARD, BOARD)
        x = _conv2d(x, self.w["board_in.weight"], self.w["board_in.bias"])
        for block in range(2):
            residual = x
            x = _group_norm(x, self.w[f"board_body.{block}.norm1.weight"],
                            self.w[f"board_body.{block}.norm1.bias"])
            x = _conv2d(_silu(x), self.w[f"board_body.{block}.conv1.weight"],
                        self.w[f"board_body.{block}.conv1.bias"], 1)
            x = _group_norm(x, self.w[f"board_body.{block}.norm2.weight"],
                            self.w[f"board_body.{block}.norm2.bias"])
            x = residual + _conv2d(
                _silu(x), self.w[f"board_body.{block}.conv2.weight"],
                self.w[f"board_body.{block}.conv2.bias"], 1)
        x = _conv2d(x, self.w["board_out.weight"], self.w["board_out.bias"])
        width = int(self.config["width"])
        x = x.reshape(1, 2, width, BOARD, BOARD).transpose(0, 1, 3, 4, 2)
        x += self.w["owner_embedding.weight"][None, :, None, None, :]
        x += self.w["x_embedding.weight"][None, None, None, :, :]
        x += self.w["y_embedding.weight"][None, None, :, None, :]
        return x.reshape(1, 2 * BOARD * BOARD, width)

    def forward(self, obs: dict[str, np.ndarray],
                state: tuple[np.ndarray, np.ndarray] | None = None) -> dict[str, np.ndarray | tuple[np.ndarray, np.ndarray]]:
        board = self._board_tokens(obs["tiles"])
        positions = obs["positions"].astype(np.int64)
        valid = obs["active_units"].astype(bool)
        clamped = np.clip(positions, 0, BOARD - 1)
        indices = clamped[..., 1] * BOARD + clamped[..., 0]
        indices = np.where(valid, indices, BOARD * BOARD)
        units = self.w["unit_position.weight"][indices]
        units += self.w["unit_owner.weight"][None, :, None, :]
        inventory_features = np.concatenate((
            _symlog(obs["own_inventory"].astype(np.float32)),
            obs["own_inventory_order"].astype(np.float32) / N_ITEMS,
        ), axis=-1)
        units[:, 0] += self._mlp(inventory_features, "inventory_encoder")
        unit_tokens = units.reshape(1, 2 * MAX_UNITS, -1)
        market = self._mlp(_symlog(obs["market"].astype(np.float32)), "market_encoder")
        market += self.w["product_embedding.weight"][None]
        divisor = np.array([720.0, 30.0, 24.0, 1.0], np.float32)
        global_values = np.concatenate((
            _symlog(obs["farm"].astype(np.float32)).reshape(1, -1),
            _symlog(obs["own_shed"].astype(np.float32)),
            _symlog(obs["own_seeds"].astype(np.float32)), obs["shops"].astype(np.float32),
            obs["clock"].astype(np.float32) / divisor,
        ), axis=-1)
        global_token = self._mlp(global_values, "global_encoder")[:, None]
        entities = np.concatenate((board, unit_tokens, market, global_token), axis=1)
        padding = np.zeros((entities.shape[0], entities.shape[1]), dtype=np.bool_)
        unit_start = 2 * BOARD * BOARD
        padding[:, unit_start:unit_start + 2 * MAX_UNITS] = ~valid.reshape(
            valid.shape[0], -1)
        batch = entities.shape[0]
        latent = np.broadcast_to(
            self.w["latents"][None], (batch, *self.w["latents"].shape)).copy()
        query = _layer_norm(latent, self.w["cross_norm_q.weight"], self.w["cross_norm_q.bias"])
        kv = _layer_norm(entities, self.w["cross_norm_kv.weight"], self.w["cross_norm_kv.bias"])
        latent += _attention(query, kv, self.w, "cross_attention",
                             int(self.config["heads"]), padding)
        for block in range(int(self.config["layers"])):
            prefix = f"blocks.{block}"
            normalized = _layer_norm(latent, self.w[f"{prefix}.attn_norm.weight"],
                                     self.w[f"{prefix}.attn_norm.bias"])
            latent += _attention(normalized, normalized, self.w, f"{prefix}.attn",
                                 int(self.config["heads"]))
            normalized = _layer_norm(latent, self.w[f"{prefix}.ff_norm.weight"],
                                     self.w[f"{prefix}.ff_norm.bias"])
            ff = _linear(normalized, self.w[f"{prefix}.ff.0.weight"],
                         self.w[f"{prefix}.ff.0.bias"])
            left, right = np.split(ff, 2, axis=-1)
            ff = left * _sigmoid(right)
            latent += _linear(ff, self.w[f"{prefix}.ff.2.weight"],
                              self.w[f"{prefix}.ff.2.bias"])
        latent = _layer_norm(latent, self.w["final_norm.weight"], self.w["final_norm.bias"])
        pooled = latent.mean(axis=1)
        if state is None:
            state = self.initial_state()
        hourly = _gru_cell(pooled, state[0], self.w, "hourly_memory")
        daily_candidate = _gru_cell(hourly, state[1], self.w, "daily_memory")
        daily = daily_candidate if int(obs["clock"][0, 2]) == 0 else state[1]
        context = self._mlp(np.concatenate((pooled, hourly, daily), axis=-1), "context")
        own_units = unit_tokens[:, :MAX_UNITS]
        worker = np.concatenate((own_units, np.broadcast_to(context[:, None], own_units.shape)), axis=-1)
        market_hidden = _gru_cell(context, context, self.w, "market_gru")
        return {
            "unit_op_logits": _linear(worker, self.w["worker_op.weight"], self.w["worker_op.bias"]),
            "unit_item_logits": _linear(worker, self.w["worker_item.weight"],
                                        self.w["worker_item.bias"]).reshape(1, MAX_UNITS, N_UNIT_OPS, N_ITEMS),
            "unit_quantity_logits": _linear(worker, self.w["worker_quantity.weight"],
                                            self.w["worker_quantity.bias"]),
            "market_hidden": market_hidden,
            "market_op_logits": _linear(market_hidden, self.w["market_op.weight"], self.w["market_op.bias"]),
            "market_item_logits": _linear(market_hidden, self.w["market_item.weight"], self.w["market_item.bias"]),
            "market_quantity_logits": _linear(market_hidden, self.w["market_quantity.weight"],
                                               self.w["market_quantity.bias"]),
            "state": (hourly, daily),
        }

    def market_step(self, hidden: np.ndarray, operation: int, item: int) -> dict[str, np.ndarray]:
        feedback = self.w["market_feedback.weight"][operation * N_ITEMS + item][None]
        hidden = _gru_cell(feedback, hidden, self.w, "market_gru")
        return {"hidden": hidden,
                "op_logits": _linear(hidden, self.w["market_op.weight"], self.w["market_op.bias"]),
                "item_logits": _linear(hidden, self.w["market_item.weight"], self.w["market_item.bias"]),
                "quantity_logits": _linear(hidden, self.w["market_quantity.weight"],
                                            self.w["market_quantity.bias"])}
