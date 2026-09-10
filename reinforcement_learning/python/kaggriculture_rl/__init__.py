"""Learned-policy tooling for Kaggriculture."""

from .model import ActorState, KaggricultureActor, ModelConfig
from .progress import Progress
from .policy import (ActionBatch, MaskBatch, PolicyBatch, act_batch,
                     evaluate_actions, joint_engine_arrays)

try:
    from ._core import ENGINE_VERSION, ReplayCursor, VectorEnv, market_price
except ImportError:  # The model and documentation remain importable before CMake build.
    ENGINE_VERSION = None
    ReplayCursor = None
    VectorEnv = None
    market_price = None

__all__ = [
    "ActorState",
    "ActionBatch",
    "ENGINE_VERSION",
    "KaggricultureActor",
    "ModelConfig",
    "MaskBatch",
    "PolicyBatch",
    "Progress",
    "ReplayCursor",
    "VectorEnv",
    "market_price",
    "act_batch",
    "evaluate_actions",
    "joint_engine_arrays",
]
