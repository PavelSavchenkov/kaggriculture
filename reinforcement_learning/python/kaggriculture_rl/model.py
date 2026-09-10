"""Compact spatial/entity transformer actor with two-timescale memory."""

from __future__ import annotations

from dataclasses import dataclass
from typing import NamedTuple

import torch
from torch import Tensor, nn
import torch.nn.functional as F

from . import schema


def symlog(value: Tensor) -> Tensor:
    return value.sign() * torch.log1p(value.abs())


@dataclass(frozen=True)
class ModelConfig:
    # Increment when actor-visible tensor semantics or required keys change.
    # This deliberately prevents resuming pre-order checkpoints whose inventory
    # encoder cannot represent capacity-sensitive DROP behavior.
    observation_version: int = 2
    width: int = 192
    board_width: int = 64
    latent_tokens: int = 24
    layers: int = 4
    heads: int = 6
    ff_mult: int = 4
    memory_width: int = 192


class ActorState(NamedTuple):
    hourly: Tensor
    daily: Tensor


class ResidualConv(nn.Module):
    def __init__(self, width: int):
        super().__init__()
        self.norm1 = nn.GroupNorm(8, width)
        self.conv1 = nn.Conv2d(width, width, 3, padding=1)
        self.norm2 = nn.GroupNorm(8, width)
        self.conv2 = nn.Conv2d(width, width, 3, padding=1)

    def forward(self, x: Tensor) -> Tensor:
        y = self.conv1(F.silu(self.norm1(x)))
        y = self.conv2(F.silu(self.norm2(y)))
        return x + y


class LatentBlock(nn.Module):
    def __init__(self, config: ModelConfig):
        super().__init__()
        d = config.width
        self.attn_norm = nn.LayerNorm(d)
        self.attn = nn.MultiheadAttention(d, config.heads, batch_first=True)
        self.ff_norm = nn.LayerNorm(d)
        self.ff = nn.Sequential(
            nn.Linear(d, config.ff_mult * d * 2),
            nn.GLU(dim=-1),
            nn.Linear(config.ff_mult * d, d),
        )

    def forward(self, x: Tensor) -> Tensor:
        z = self.attn_norm(x)
        x = x + self.attn(z, z, z, need_weights=False)[0]
        return x + self.ff(self.ff_norm(x))


class KaggricultureActor(nn.Module):
    """Observation-only actor. It is safe to export to a live agent."""

    def __init__(self, config: ModelConfig = ModelConfig()):
        super().__init__()
        self.config = config
        d, bw = config.width, config.board_width

        # Tile fields are converted to one-hots plus continuous lifecycle values.
        tile_in = 6 + schema.N_ITEMS + 5 + 6
        self.board_in = nn.Conv2d(tile_in, bw, 1)
        self.board_body = nn.Sequential(ResidualConv(bw), ResidualConv(bw))
        self.board_out = nn.Conv2d(bw, d, 1)
        self.owner_embedding = nn.Embedding(2, d)
        self.x_embedding = nn.Embedding(schema.BOARD, d)
        self.y_embedding = nn.Embedding(schema.BOARD, d)

        self.inventory_encoder = nn.Sequential(
            nn.Linear(2 * schema.N_ITEMS, d), nn.SiLU(), nn.Linear(d, d))
        self.unit_owner = nn.Embedding(2, d)
        self.unit_position = nn.Embedding(schema.BOARD * schema.BOARD + 1, d)
        self.product_embedding = nn.Embedding(schema.N_PRODUCTS, d)
        self.market_encoder = nn.Sequential(nn.Linear(2, d), nn.SiLU(), nn.Linear(d, d))
        global_in = 2 * 4 + schema.N_ITEMS + schema.N_CROPS + schema.N_SHOPS + 4
        self.global_encoder = nn.Sequential(nn.Linear(global_in, d), nn.SiLU(), nn.Linear(d, d))

        self.latents = nn.Parameter(torch.randn(config.latent_tokens, d) * 0.02)
        self.cross_norm_q = nn.LayerNorm(d)
        self.cross_norm_kv = nn.LayerNorm(d)
        self.cross_attention = nn.MultiheadAttention(d, config.heads, batch_first=True)
        self.blocks = nn.ModuleList(LatentBlock(config) for _ in range(config.layers))
        self.final_norm = nn.LayerNorm(d)

        self.hourly_memory = nn.GRUCell(d, config.memory_width)
        self.daily_memory = nn.GRUCell(config.memory_width, config.memory_width)
        fused = d + 2 * config.memory_width
        self.context = nn.Sequential(nn.Linear(fused, d), nn.SiLU(), nn.Linear(d, d))

        worker_in = 2 * d
        self.worker_op = nn.Linear(worker_in, schema.N_UNIT_OPS)
        self.worker_item = nn.Linear(worker_in, schema.N_UNIT_OPS * schema.N_ITEMS)
        self.worker_quantity = nn.Linear(worker_in, schema.N_QUANTITIES)

        self.market_gru = nn.GRUCell(d, d)
        self.market_op = nn.Linear(d, schema.N_MARKET_OPS)
        self.market_item = nn.Linear(d, schema.N_ITEMS)
        self.market_quantity = nn.Linear(d, schema.N_QUANTITIES)
        self.market_feedback = nn.Embedding(
            schema.N_MARKET_OPS * schema.N_ITEMS, d)

        self.wdl = nn.Linear(d, 3)
        self.margin = nn.Linear(d, 1)
        self.opponent_flow = nn.Linear(d, schema.N_PRODUCTS)

    def initial_state(self, batch: int, *, device=None, dtype=None) -> ActorState:
        kwargs = {"device": device, "dtype": dtype}
        return ActorState(
            torch.zeros(batch, self.config.memory_width, **kwargs),
            torch.zeros(batch, self.config.memory_width, **kwargs),
        )

    def _board_tokens(self, tiles: Tensor) -> Tensor:
        # tiles: B, owner, Y, X, 13
        kind = F.one_hot(tiles[..., 0].long().clamp(0, 5), 6)
        item = F.one_hot(
            tiles[..., 1].long().clamp(0, schema.N_ITEMS - 1), schema.N_ITEMS)
        item_present = (tiles[..., 0].eq(5) | tiles[..., 2].bool()).unsqueeze(-1)
        item = item * item_present
        flags = tiles[..., 2:7].float()
        numeric = symlog(tiles[..., 7:13].float())
        x = torch.cat((kind, item, flags, numeric), dim=-1)
        batch = x.shape[0]
        x = x.permute(0, 1, 4, 2, 3).reshape(batch * 2, -1, schema.BOARD, schema.BOARD)
        x = self.board_out(self.board_body(self.board_in(x)))
        x = x.reshape(batch, 2, self.config.width, schema.BOARD, schema.BOARD)
        x = x.permute(0, 1, 3, 4, 2)
        owners = self.owner_embedding(torch.arange(2, device=x.device))[None, :, None, None]
        xs = self.x_embedding(torch.arange(schema.BOARD, device=x.device))[None, None, None]
        ys = self.y_embedding(torch.arange(schema.BOARD, device=x.device))[None, None, :, None]
        x = x + owners + xs + ys
        return x.reshape(batch, 2 * schema.BOARD * schema.BOARD, self.config.width)

    def _unit_tokens(self, obs: dict[str, Tensor]) -> Tensor:
        pos = obs["positions"].long()
        valid = obs["active_units"].bool()
        clamped = pos.clamp(0, schema.BOARD - 1)
        index = clamped[..., 1] * schema.BOARD + clamped[..., 0]
        index = torch.where(valid, index, torch.full_like(index, schema.BOARD * schema.BOARD))
        token = self.unit_position(index)
        token = token + self.unit_owner(torch.arange(2, device=pos.device))[None, :, None]
        own_inventory = symlog(obs["own_inventory"].float())
        inventory_order = obs["own_inventory_order"].float() / schema.N_ITEMS
        inventory_features = torch.cat((own_inventory, inventory_order), dim=-1)
        token[:, 0] = token[:, 0] + self.inventory_encoder(inventory_features)
        return token.flatten(1, 2)

    def _entity_tokens(self, obs: dict[str, Tensor]) -> tuple[Tensor, Tensor, Tensor]:
        board = self._board_tokens(obs["tiles"])
        units = self._unit_tokens(obs)
        market = symlog(obs["market"].float())
        ids = torch.arange(schema.N_PRODUCTS, device=market.device)
        market = self.market_encoder(market) + self.product_embedding(ids)[None]
        global_values = torch.cat((
            symlog(obs["farm"].float()).flatten(1),
            symlog(obs["own_shed"].float()),
            symlog(obs["own_seeds"].float()),
            obs["shops"].float(),
            obs["clock"].float() / torch.tensor(
                [720.0, 30.0, 24.0, 1.0], device=market.device),
        ), dim=-1)
        global_token = self.global_encoder(global_values)[:, None]
        entities = torch.cat((board, units, market, global_token), dim=1)
        batch = entities.shape[0]
        padding = torch.zeros(batch, entities.shape[1], dtype=torch.bool,
                              device=entities.device)
        unit_start = 2 * schema.BOARD * schema.BOARD
        padding[:, unit_start:unit_start + 2 * schema.MAX_UNITS] = ~obs[
            "active_units"].bool().flatten(1)
        return entities, units[:, :schema.MAX_UNITS], padding

    def forward(self, obs: dict[str, Tensor], state: ActorState | None = None) -> dict[str, Tensor | ActorState]:
        entities, own_units, entity_padding = self._entity_tokens(obs)
        batch = entities.shape[0]
        latent = self.latents[None].expand(batch, -1, -1)
        latent = latent + self.cross_attention(
            self.cross_norm_q(latent), self.cross_norm_kv(entities),
            self.cross_norm_kv(entities), key_padding_mask=entity_padding,
            need_weights=False)[0]
        for block in self.blocks:
            latent = block(latent)
        latent = self.final_norm(latent)
        pooled = latent.mean(dim=1)

        if state is None:
            state = self.initial_state(batch, device=pooled.device, dtype=pooled.dtype)
        hourly = self.hourly_memory(pooled, state.hourly)
        daily_candidate = self.daily_memory(hourly, state.daily)
        day_boundary = obs["clock"][:, 2].eq(0)[:, None]
        daily = torch.where(day_boundary, daily_candidate, state.daily)
        context = self.context(torch.cat((pooled, hourly, daily), dim=-1))

        worker = torch.cat((own_units, context[:, None].expand(-1, schema.MAX_UNITS, -1)), dim=-1)
        market_hidden = self.market_gru(context, context)
        return {
            "unit_op_logits": self.worker_op(worker),
            "unit_item_logits": self.worker_item(worker).view(
                batch, schema.MAX_UNITS, schema.N_UNIT_OPS, schema.N_ITEMS),
            "unit_quantity_logits": self.worker_quantity(worker),
            "market_hidden": market_hidden,
            "market_op_logits": self.market_op(market_hidden),
            "market_item_logits": self.market_item(market_hidden),
            "market_quantity_logits": self.market_quantity(market_hidden),
            "wdl_logits": self.wdl(context),
            "margin": self.margin(context).squeeze(-1),
            "opponent_flow": self.opponent_flow(context),
            "state": ActorState(hourly, daily),
        }

    def market_step(self, hidden: Tensor, operation: Tensor, item: Tensor) -> dict[str, Tensor]:
        feedback = self.market_feedback(operation * schema.N_ITEMS + item)
        hidden = self.market_gru(feedback, hidden)
        return {
            "hidden": hidden,
            "op_logits": self.market_op(hidden),
            "item_logits": self.market_item(hidden),
            "quantity_logits": self.market_quantity(hidden),
        }
