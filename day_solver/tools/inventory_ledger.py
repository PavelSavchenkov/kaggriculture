#!/usr/bin/env python3
"""Inventory-only trace of an already replay-valid v3 schedule."""
from collections import defaultdict
from types import SimpleNamespace

ITEM_NAMES = ("WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG",
              "MILK", "WOOL", "FERTILIZER", "GOOSE", "COW", "SHEEP")
SHED_ACCESS = ((4, 4), (5, 4), (4, 5), (5, 5))


def compile_tasks(problem):
    tasks = []
    for work in problem["tile_work"]:
        tile = problem["start"]["managed_tiles"][work["tile"]]
        for value in work["actions"]:
            op, arg = value["op"], value["arg"]
            action = (op.upper(), ITEM_NAMES[arg]) if op in ("plant", "place") else (op.upper(),)
            tasks.append(SimpleNamespace(id=len(tasks), x=tile["x"], y=tile["y"], action=action,
                input_item=0 if op == "feed" else 8 if op == "fertilize" else arg if op == "place" else -1,
                seed_crop=arg if op == "plant" else -1, output_item=value["output_item"],
                output_quantity=value["output_quantity"]))
    return tasks


MOVES = {"EAST": (1, 0), "WEST": (-1, 0), "NORTH": (0, -1), "SOUTH": (0, 1)}


def trace(problem, schedule):
    assert problem["format_version"] == 3 and len(schedule) == 24
    tasks = compile_tasks(problem)
    at = defaultdict(list)
    for task in tasks:
        at[task.x, task.y].append(task)
    next_task = defaultdict(int)
    purchases = defaultdict(list)
    for buy in sorted(problem["buy_schedule"], key=lambda row: (row["hour"], row["order_index"])):
        purchases[buy["hour"]].append(buy)
    shed = list(problem["start"]["shed"])
    seeds = list(problem["start"]["seeds"])
    positions = [SHED_ACCESS[0]]
    cargo = [[0] * 12]
    events, hours, deposits = [], [], []
    completed_tasks = 0
    for hour, turn in enumerate(schedule):
        assert len(turn["units"]) == len(positions)
        previous = problem["shed_availability"][hour - 1] if hour else [0] * 12
        remaining = [last - used for last, used in zip(problem["shed_availability"][-1], previous)]
        for worker, action in enumerate(turn["units"]):
            point = positions[worker]
            op = action[0]
            if op == "PASS":
                continue
            if op in MOVES:
                dx, dy = MOVES[op]
                positions[worker] = point[0] + dx, point[1] + dy
                continue
            event = {"hour": hour, "worker": worker, "point": point, "action": action,
                     "cargo_before": list(cargo[worker])}
            candidates = at[point]
            index = next_task[point]
            if index < len(candidates) and tuple(action) == candidates[index].action:
                task = candidates[index]
                event["task"] = task.id
                if task.input_item >= 0:
                    cargo[worker][task.input_item] -= 1
                if task.output_item >= 0:
                    cargo[worker][task.output_item] += task.output_quantity
                if task.seed_crop >= 0:
                    seeds[task.seed_crop] -= 1
                next_task[point] += 1
                completed_tasks += 1
            elif op == "PICKUP":
                assert point in SHED_ACCESS
                item = ITEM_NAMES.index(action[1])
                quantity = min(action[2], shed[item])
                assert quantity > 0
                cargo[worker][item] += quantity
                shed[item] -= quantity
                event["picked"] = {item: quantity}
            elif op in ("PLACE", "DROP"):
                assert point in SHED_ACCESS
                if op == "PLACE":
                    item = ITEM_NAMES.index(action[1])
                    quantity = min(action[2], cargo[worker][item])
                    assert quantity > 0
                    units = {item: quantity}
                else:
                    units = {item: quantity for item, quantity in enumerate(cargo[worker]) if quantity}
                    assert units
                for item, quantity in units.items():
                    cargo[worker][item] -= quantity
                    shed[item] += quantity
                event["deposited"] = units
                event["remaining_timed_demand_for_deposited_items"] = {
                    item: remaining[item] for item in units
                }
                event["can_support_remaining_withdrawal"] = any(remaining[item] for item in units)
                deposits.append(event)
            else:
                raise ValueError(f"Unmapped action at h{hour}, worker {worker}: {action}")
            assert min(cargo[worker]) >= 0 and min(shed) >= 0 and min(seeds) >= 0
            event["cargo_after"] = list(cargo[worker])
            events.append(event)
        before_availability = list(shed)
        for item in range(12):
            shed[item] -= problem["shed_availability"][hour][item] - previous[item]
        assert min(shed) >= 0
        after_availability = list(shed)
        for buy in purchases[hour]:
            if buy["op"] == "hire":
                for _ in range(buy["quantity"]):
                    positions.append(min(SHED_ACCESS, key=positions.count))
                    cargo.append([0] * 12)
            elif buy["op"] == "buy_seed":
                seeds[buy["item"]] += buy["quantity"]
            elif buy["op"] in ("buy_product", "buy_animal"):
                shed[buy["item"]] += buy["quantity"]
            elif buy["op"] != "buy_land":
                raise ValueError(f"Unexpected purchase: {buy}")
        hours.append({"hour": hour, "shed_before_availability": before_availability,
                      "shed_after_availability": after_availability,
                      "shed_after_market": list(shed)})
    assert completed_tasks == len(tasks)
    assert len(positions) == problem["worker_count"]
    final = [quantity + sum(worker[item] for worker in cargo) for item, quantity in enumerate(shed)]
    assert final == problem["end_shed"] and seeds == problem["end_seeds"]
    return {"events": events, "hours": hours, "deposits": deposits, "end_shed": final,
            "end_seeds": seeds, "tasks": completed_tasks}


def compare_auditor(ledger, audit):
    assert all(audit[key] for key in ("strict", "requirements", "invariants"))
    for ours, theirs in zip(ledger["hours"], audit["hours"], strict=True):
        assert ours["hour"] == theirs["hour"]
        # The auditor's historical field name includes the availability
        # withdrawal, although it is named shed_after_workers.
        assert ours["shed_after_availability"] == theirs["shed_after_workers"], ours["hour"]
        assert ours["shed_after_market"] == theirs["shed_after_market"], ours["hour"]
    assert ledger["end_shed"] == audit["end_state"]["physical"]["shed"]
    assert ledger["end_seeds"] == audit["end_state"]["physical"]["seeds"]
