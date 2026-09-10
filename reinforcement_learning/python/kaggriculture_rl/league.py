"""Small deterministic league primitives; storage and orchestration come later."""

from __future__ import annotations

import numpy as np


def pfsp_probabilities(win_rates, *, mode: str = "variance", floor: float = 0.02):
    """Return prioritized-fictitious-self-play opponent probabilities.

    `win_rates[i]` is the candidate's measured win rate against opponent i.
    Variance weighting emphasizes competitive opponents; squared weighting
    emphasizes exploiters that currently beat the candidate.
    """
    rates = np.asarray(win_rates, dtype=np.float64)
    if rates.ndim != 1 or rates.size == 0:
        raise ValueError("win_rates must be a non-empty one-dimensional sequence")
    if np.any((rates < 0) | (rates > 1)):
        raise ValueError("win rates must lie in [0, 1]")
    if mode == "variance":
        weights = rates * (1.0 - rates)
    elif mode == "squared":
        weights = (1.0 - rates) ** 2
    else:
        raise ValueError(f"unknown PFSP mode: {mode}")
    weights = weights + float(floor)
    return weights / weights.sum()
