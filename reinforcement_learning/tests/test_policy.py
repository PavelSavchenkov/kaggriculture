import numpy as np
import torch

from kaggriculture_rl import (
    KaggricultureActor,
    VectorEnv,
    act_batch,
    evaluate_actions,
    joint_engine_arrays,
    market_price,
)
from kaggriculture_rl.action_codec import teacher_masks


def _torch_rows(rows):
    return {name: torch.from_numpy(value) for name, value in rows.items()}


def test_sampled_policy_log_probability_and_masks_are_exact():
    env = VectorEnv([101, 102])
    observations = env.observe(0)
    model = KaggricultureActor().eval()
    sampled = act_batch(
        model, observations, None, device=torch.device("cpu"),
        rng=np.random.default_rng(7), price_function=market_price)
    actions = {name: torch.from_numpy(value)
               for name, value in sampled.actions.arrays().items()}
    masks = {name: torch.from_numpy(value)
             for name, value in sampled.masks.arrays().items()}
    with torch.no_grad():
        log_prob, _, _ = evaluate_actions(
            model, _torch_rows(observations), None, actions, masks)
    assert np.allclose(log_prob.numpy(), sampled.log_prob, atol=2e-4)

    for row in range(2):
        action = {name: value[row] for name, value in sampled.actions.arrays().items()}
        one = {name: value[row:row + 1] for name, value in observations.items()}
        expected = teacher_masks(one, action, market_price)
        for name, value in sampled.masks.arrays().items():
            assert np.array_equal(value[row], expected[name])


def test_sampled_joint_actions_step_vector_engine():
    env = VectorEnv([201, 202], episode_steps=8)
    model = KaggricultureActor().eval()
    first = act_batch(
        model, env.observe(0), None, device=torch.device("cpu"),
        rng=np.random.default_rng(11), deterministic=True,
        price_function=market_price)
    second = act_batch(
        model, env.observe(1), None, device=torch.device("cpu"),
        rng=np.random.default_rng(12), deterministic=True,
        price_function=market_price)
    status = env.step(*joint_engine_arrays(first.actions, second.actions))
    assert status["money"].shape == (2, 2)
    assert not status["done"].any()
