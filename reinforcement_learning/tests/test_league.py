import numpy as np
import pytest

from kaggriculture_rl.league import pfsp_probabilities


def test_variance_pfsp_prefers_competitive_opponents():
    p = pfsp_probabilities([0.05, 0.5, 0.95])
    assert np.isclose(p.sum(), 1.0)
    assert p[1] > p[0]
    assert np.isclose(p[0], p[2])


def test_pfsp_rejects_invalid_rates():
    with pytest.raises(ValueError):
        pfsp_probabilities([1.1])
