from pathlib import Path

import numpy as np
import pytest
import torch

from kaggriculture_rl import KaggricultureActor, ReplayCursor, VectorEnv, market_price
from kaggriculture_rl.action_codec import (
    MARKET_END,
    PlanningState,
    decode_action,
    teacher_masks,
    unpack_mask,
)
from kaggriculture_rl.portable_runtime import PortableActor


def test_teacher_targets_are_inside_shared_masks():
    paths = sorted(Path("corpus/data").glob("*.kagz"))
    if not paths:
        pytest.skip("small replay corpus is absent")
    cursor = ReplayCursor(str(paths[0]))
    for _ in range(12):
        for seat in (0, 1):
            observation = cursor.observe(seat)
            action = cursor.joint_effective_action(seat)
            action["n_orders"] = 0
            masks = teacher_masks(observation, action, market_price)
            for unit in range(int(action["n_units"])):
                assert unpack_mask(masks["unit_op_bits"][unit], 18)[
                    int(action["unit_op"][unit])]
            assert unpack_mask(masks["market_op_bits"][-1], 8)[MARKET_END]
        cursor.advance()


def test_structured_decode_matches_portable_runtime():
    environment = VectorEnv([703])
    observation = environment.observe(0)
    model = KaggricultureActor().eval()
    arrays = {key: value.detach().numpy().astype(np.float32, copy=False)
              for key, value in model.state_dict().items()}
    portable = PortableActor(arrays, model.config.__dict__)
    with torch.inference_mode():
        raw = model({key: torch.from_numpy(value) for key, value in observation.items()})
    expected = {key: value.detach().numpy() for key, value in raw.items()
                if isinstance(value, torch.Tensor)}

    def market_step(hidden, operation, item):
        with torch.inference_mode():
            result = model.market_step(torch.from_numpy(hidden), torch.tensor([operation]),
                                       torch.tensor([item]))
        return {key: value.numpy() for key, value in result.items()}

    expected["market_step"] = market_step
    actual = portable.forward(observation)
    actual["market_step"] = portable.market_step
    assert decode_action(expected, observation, market_price) == decode_action(
        actual, observation, market_price)


def test_drop_respects_inventory_insertion_order_at_shed_capacity():
    environment = VectorEnv([703])
    observation = environment.observe(0)
    observation["own_shed"][0, 11] = 95
    observation["own_inventory"][0, 0, 0] = 4
    observation["own_inventory"][0, 0, 8] = 3
    observation["own_inventory_order"][0, 0, 8] = 1
    observation["own_inventory_order"][0, 0, 0] = 2
    state = PlanningState.from_observation(observation, shed_capacity=100)

    state.apply_unit(0, 6)

    assert state.shed[8] == 3
    assert state.shed[0] == 2
    assert state.shed[11] == 95
    assert not state.inventories[0].any()
    assert not state.inventory_order[0].any()
