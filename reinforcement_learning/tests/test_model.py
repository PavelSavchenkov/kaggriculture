import torch

from kaggriculture_rl import KaggricultureActor
from kaggriculture_rl._core import VectorEnv


def test_actor_shapes_and_recurrent_state():
    env = VectorEnv([11, 12])
    obs = {name: torch.from_numpy(value) for name, value in env.observe(0).items()}
    model = KaggricultureActor()
    out = model(obs)
    assert out["unit_op_logits"].shape == (2, 40, 18)
    assert out["unit_item_logits"].shape == (2, 40, 18, 12)
    assert out["market_op_logits"].shape == (2, 8)
    assert out["wdl_logits"].shape == (2, 3)
    again = model(obs, out["state"])
    assert not torch.equal(out["state"].hourly, again["state"].hourly)
