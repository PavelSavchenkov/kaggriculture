import numpy as np
import torch

from kaggriculture_rl import KaggricultureActor, ModelConfig, VectorEnv, market_price
from kaggriculture_rl.selfplay import (
    CentralCritic,
    OpponentEntry,
    OpponentPool,
    central_features,
    collect_rollout,
    ppo_update,
)


def tiny_actor():
    return KaggricultureActor(ModelConfig(
        width=48, board_width=16, latent_tokens=4, layers=1,
        heads=4, ff_mult=2, memory_width=48))


def test_short_exact_engine_rollout_and_recurrent_ppo_update():
    torch.manual_seed(31)
    actor = tiny_actor()
    opponent = tiny_actor()
    opponent.load_state_dict(actor.state_dict())
    environment = VectorEnv([301, 302], episode_steps=8)
    width = central_features(
        environment.observe(0), environment.observe(1)).shape[1]
    critic = CentralCritic(width, width=64)
    actor_optimizer = torch.optim.AdamW(actor.parameters(), lr=1e-5)
    critic_optimizer = torch.optim.AdamW(critic.parameters(), lr=1e-4)
    before = actor.worker_op.weight.detach().clone()
    rollout = collect_rollout(
        actor, opponent, critic, environment, learner_seat=0,
        device=torch.device("cpu"), rng=np.random.default_rng(33),
        temperature=1.0, price_function=market_price, episode_steps=8)
    assert rollout.steps == 7
    assert rollout.environments == 2
    assert rollout.parity_hashes.shape == (8, 2)
    assert rollout.joint_actions["unit_op"].shape == (7, 2, 2, 40)
    assert set(np.unique(rollout.outcomes)).issubset({-1.0, 0.0, 1.0})
    metrics = ppo_update(
        actor, critic, actor_optimizer, critic_optimizer, rollout,
        device=torch.device("cpu"), rng=np.random.default_rng(34),
        sequence_length=4, sequences_per_minibatch=2, epochs=1,
        clip_ratio=0.1, value_coefficient=0.5,
        entropy_coefficient=1e-4, max_grad_norm=0.5, temperature=1.0)
    assert all(np.isfinite(value) for value in metrics.values())
    assert not torch.equal(before, actor.worker_op.weight)


def test_opponent_pool_round_trip_and_probabilities():
    pool = OpponentPool([
        OpponentEntry("a", "/tmp/a.pt"),
        OpponentEntry("b", "/tmp/b.pt"),
    ])
    pool.record("a", np.asarray((1.0, -1.0, 0.0), dtype=np.float32))
    probabilities = pool.probabilities()
    assert np.isclose(probabilities.sum(), 1.0)
    restored = OpponentPool.restore(pool.state())
    assert restored.entries[0].games == 3
    assert restored.entries[0].score == 1.5
