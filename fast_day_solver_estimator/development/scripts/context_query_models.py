"""Cold-call features depend only on work, horizon and selected hire times."""
import numpy as np

from query_models import CpuModel, SuccessModel


def design(x, workers, names):
    x = np.asarray(x, dtype=np.float32).astype(np.float64)
    workers = np.asarray(workers, dtype=int)
    ix = {name: i for i, name in enumerate(names)}
    hours = x[:, ix["active_hours"]].astype(int)
    available_name = "available_hire_slots" if "available_hire_slots" in ix else "optional_hire_slots"
    prefix = "selection_birth_" if "selection_birth_0" in ix else "optional_birth_"
    births = x[:, [ix[prefix + str(i)] for i in range(39)]].astype(int)
    minimum = x[:, ix["committed_workforce"]] if "committed_workforce" in ix else np.ones(len(x))
    assert np.all(np.isin(hours, [23, 24])) and np.all(workers >= minimum) and np.all(workers <= x[:, ix[available_name]] + 1)
    selected = np.arange(39)[None, :] < (workers[:, None] - 1)
    # The first 233 columns are the original physical features. Neither full-
    # menu bounds nor optional menu shape may leak into an individual call.
    assert names[232] == "deadline_deficit_4" and names[233] == "active_hours"
    columns = [x[:, :233], hours[:, None], workers[:, None]]
    capacities = []
    for h in range(24):
        end = np.minimum(h + 1, hours)
        capacity = end + np.sum(np.maximum(0, end[:, None] - births) * selected, axis=1)
        capacities.append(capacity)
        columns.append(capacity[:, None])
    for radius in range(9):
        capacity = np.maximum(0, hours - radius) + np.sum(np.maximum(0, hours[:, None] - births - radius) * selected, axis=1)
        columns.append(capacity[:, None])
    for h in range(24):
        columns.append((x[:, ix[f"deadline_required_actions_{h}"]] / np.maximum(1, capacities[h]))[:, None])
    for name in ["tasks", "task_motion_bound", "task_distance", "rooted_mst", "input_actions", "output_actions",
                 "route_pack_open", "route_pack_return", "deadline_pressure_max", "required_pickup_types"]:
        values = x[:, ix[name]]
        columns.extend([(values / workers)[:, None], (values / np.maximum(1, capacities[-1]))[:, None]])
    for name in ["supply_missing", "supply_late_actions", "supply_pressure", "seed_missing", "land_missing", "seed_late_actions", "land_late_actions", "release_pressure"]:
        columns.append(x[:, ix[name]][:, None])
    result = np.column_stack(columns)
    assert result.shape[1] == 320
    return result


def candidates():
    return {"logistic": SuccessModel("logistic"), "boost": SuccessModel("boost"), "cpu": CpuModel()}
