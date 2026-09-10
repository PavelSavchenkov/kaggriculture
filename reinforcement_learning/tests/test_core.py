from pathlib import Path

import numpy as np
import pytest

from kaggriculture_rl._core import ENGINE_VERSION, ReplayCursor, VectorEnv


def test_vector_env_actor_observation_is_perspective_relative():
    env = VectorEnv([1, 2, 3])
    own = env.observe(0)
    other = env.observe(1)
    assert ENGINE_VERSION == "1.32.7"
    assert own["tiles"].shape == (3, 2, 10, 10, 13)
    assert own["positions"].shape == (3, 2, 40, 2)
    assert own["own_inventory"].shape == (3, 40, 12)
    assert own["own_inventory_order"].shape == (3, 40, 12)
    assert own["market"].shape == (3, 9, 2)
    assert np.array_equal(own["farm"][:, 0], other["farm"][:, 1])
    assert np.array_equal(own["farm"][:, 1], other["farm"][:, 0])
    env.pass_step()
    assert np.all(env.observe(0)["clock"][:, 0] == 1)


def test_vector_env_steps_structured_joint_actions():
    env = VectorEnv([9])
    unit_op = np.zeros((1, 2, 40), dtype=np.uint8)
    unit_arg = np.zeros_like(unit_op)
    unit_n = np.ones((1, 2, 40), dtype=np.int32)
    n_units = np.ones((1, 2), dtype=np.int32)
    order_op = np.zeros((1, 2, 10), dtype=np.uint8)
    order_item = np.zeros_like(order_op)
    order_n = np.zeros((1, 2, 10), dtype=np.int32)
    n_orders = np.zeros((1, 2), dtype=np.int32)
    unit_op[0, 0, 0] = 3  # EAST
    env.step(unit_op, unit_arg, unit_n, n_units,
             order_op, order_item, order_n, n_orders)
    observation = env.observe(0)
    assert tuple(observation["positions"][0, 0, 0]) == (5, 4)
    assert tuple(observation["positions"][0, 1, 0]) == (4, 4)


def test_vector_env_configurable_short_episode():
    env = VectorEnv([17, 18], episode_steps=8)
    initial_hash = env.status()["parity_hash"].copy()
    for _ in range(7):
        status = env.pass_step()
    assert np.all(status["done"])
    assert np.all(status["money"] == 3000)
    assert status["parity_hash"].dtype == np.uint64
    assert np.all(status["parity_hash"] != initial_hash)


def test_replay_cursor_uses_next_action_and_verifies():
    paths = sorted(Path("corpus/data").glob("*.kagz"))
    if not paths:
        pytest.skip("small replay corpus is absent")
    cursor = ReplayCursor(str(paths[0]))
    assert cursor.step == 0
    assert cursor.n_steps == 720
    assert cursor.verify()
    assert cursor.verify_joint_canonicalization()
    assert isinstance(cursor.submission_ids, list)
    action = cursor.recorded_action(0)
    effective = cursor.joint_effective_action(0)
    assert action["unit_op"].shape == (40,)
    assert effective["order_n"].shape == (16,)
    assert cursor.advance()
    assert cursor.step == 1
    assert cursor.observe(0)["clock"][0, 0] == 1
