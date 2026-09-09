"""Cheap predictors for a thirty-second retry after an observed three-second failure."""
import numpy as np

from context_query_models import design as context_design


PRIOR_NAMES = ["has_prior_certificate", "prior_upper_or_41", "workers_below_prior_upper", "relative_gap_to_prior_upper",
               "log1p_failed_short_cpu", "log1p_failed_short_wall", "proposed_workers"]


def design(rows, names, view):
    workers = np.array([r["workers"] for r in rows])
    prior = np.array([[r["prior_upper_workers"] is not None, r["prior_upper_workers"] if r["prior_upper_workers"] is not None else 41,
                       r["prior_upper_workers"] - r["workers"] if r["prior_upper_workers"] is not None else 0,
                       (r["prior_upper_workers"] - r["workers"]) / r["workers"] if r["prior_upper_workers"] is not None else 0,
                       np.log1p(r["prior_short_cpu_seconds"]), np.log1p(r["prior_short_wall_seconds"]), r["workers"]] for r in rows], dtype=float)
    assert np.isfinite(prior).all() and np.all(workers >= 1) and np.all(workers <= 40)
    if view == "prior_only": return prior
    physical = context_design(np.array([r["features"] for r in rows]), workers, names)
    if view == "physical": return physical
    if view == "with_prior": return np.column_stack([physical, prior])
    raise ValueError(view)
