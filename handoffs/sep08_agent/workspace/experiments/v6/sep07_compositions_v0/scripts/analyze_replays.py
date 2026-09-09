import argparse
import copy
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path

from kaggle_environments.envs.kaggriculture import kaggriculture as game


ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "research"
MANIFEST = WORK / "top_replay_manifest.csv"
REPLAYS = ROOT / "replays"
PRODUCTS = (
    "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK",
    "WOOL", "FERTILIZER",
)
ANIMALS = ("GOOSE", "COW", "SHEEP")
ANIMAL_PRODUCT = {"GOOSE": "EGG", "COW": "MILK", "SHEEP": "WOOL"}
ANIMAL_CAP = {"GOOSE": 4, "COW": 6, "SHEEP": 6}
STRUCTURE = {"GOOSE": "COOP", "COW": "PASTURE", "SHEEP": "PASTURE"}
BONUS_AGES = {
    "WHEAT": {2, 3, 4},
    "CARROT": {2, 3},
    "MELON": set(range(6, 13)),
    "TOMATO": {7, 8, 9, 10},
    "STRAWBERRY": {9, 11, 13, 15},
}
SHOP_PRODUCTS = {
    "BAKERY": ("EGG", "WHEAT"),
    "PIZZA_SHOP": ("MILK", "TOMATO", "WHEAT"),
    "BRUNCH_SPOT": ("EGG", "WHEAT", "STRAWBERRY"),
    "YARN_STORE": ("WOOL", "WOOL"),
    "ICE_CREAM_SHOP": ("STRAWBERRY", "MILK", "WHEAT"),
    "PET_CAFE": ("CARROT", "CARROT"),
    "SMOOTHIE_SHOP": ("STRAWBERRY", "MILK"),
    "FARMERS_MARKET": ("WHEAT", "CARROT", "TOMATO", "STRAWBERRY"),
}


def write_csv(name, rows):
    if not rows:
        raise RuntimeError(f"no rows for {name}")
    path = WORK / name
    with path.open("w", newline="") as target:
        writer = csv.DictWriter(target, fieldnames=rows[0])
        writer.writeheader()
        writer.writerows(rows)


def tile_at(farm, x, y):
    return farm["tiles"][y][x]


def unit_position(farm, index):
    return farm["farmer"] if index == 0 else farm["hands"][index - 1]


def is_crop(tile):
    return isinstance(tile, dict) and tile.get("kind") == "PLANT"


def has_animal(tile):
    return isinstance(tile, dict) and bool(tile.get("animal"))


def occupant(tile):
    if is_crop(tile):
        return ("crop", tile["crop"], tile["planted_day"])
    if has_animal(tile):
        return ("animal", tile["animal"], tile["placed_day"])
    return None


def center_distance(x, y, board_size=10):
    centers = (board_size // 2 - 1, board_size // 2)
    return min(abs(x - cx) + abs(y - cy) for cx in centers for cy in centers)


def shop_demand(shops):
    demand = Counter()
    for shop in shops:
        demand.update(SHOP_PRODUCTS[shop])
    return demand


def inventory_count(inventories, item):
    return sum(inventory.get(item, 0) for inventory in inventories)


def portable_total(private):
    return sum(private["shed"].values()) + sum(
        sum(inventory.values()) for inventory in private["inventories"]
    )


def apply_units(farms, privates, actions, day, result_index, contexts, unit_events):
    for player in range(2):
        farm = farms[player]
        private = privates[player]
        action = actions[player]
        commands = [action.get("farmer", ["PASS"]), *action.get("hands", [])]
        plant_demand = Counter(
            command[1] for command in commands
            if isinstance(command, list) and len(command) >= 2 and command[0] == "PLANT"
        )
        blocked = {
            crop for crop, count in plant_demand.items()
            if count > private["seeds"].get(crop, 0)
        }
        context = contexts.get(player)
        for index, original in enumerate(commands):
            if index > len(farm["hands"]):
                continue
            command = original
            if (
                isinstance(command, list) and len(command) >= 2
                and command[0] == "PLANT" and command[1] in blocked
            ):
                command = ["PASS"]
            if not isinstance(command, list) or not command:
                continue
            x, y = unit_position(farm, index)
            before_tile = copy.deepcopy(tile_at(farm, x, y))
            before_shed = sum(private["shed"].values())
            before_inventory = sum(private["inventories"][index].values())
            before_fertilizer = private["inventories"][index].get("FERTILIZER", 0)
            before_items = dict(private["inventories"][index])
            before_seeds = dict(private["seeds"])
            before_stock = dict(private["shed"])
            before = occupant(before_tile)
            game._apply_unit_action(farm, private, index, command, 10, day, 24, 100)
            after_tile = tile_at(farm, x, y)
            after = occupant(after_tile)
            if context is not None and original[0] != "PASS":
                context["unit_requests"] += 1
                changed = (
                    before_tile != after_tile or (x, y) != tuple(unit_position(farm, index))
                    or before_items != private["inventories"][index]
                    or before_seeds != private["seeds"] or before_stock != private["shed"]
                )
                if not changed:
                    context["unit_faults"][original[0]] += 1
            op = command[0]
            success = False
            quantity = 0
            source = ""
            if op == "PLANT":
                success = after is not None and after != before and after[0] == "crop"
                source = command[1] if len(command) > 1 else ""
            elif op == "WATER":
                success = (
                    is_crop(before_tile) and not before_tile["watered_today"]
                    and is_crop(after_tile) and after_tile["watered_today"]
                )
                source = before_tile.get("crop", "") if isinstance(before_tile, dict) else ""
            elif op == "HARVEST" and before is not None:
                quantity = before_tile.get("yield_units", 0)
                success = quantity > 0 and (
                    after != before or after_tile.get("yield_units", 0) == 0
                )
                source = before[1]
            elif op == "FERTILIZE":
                success = private["inventories"][index].get("FERTILIZER", 0) < before_fertilizer
                source = before_tile.get("crop", "") if is_crop(before_tile) else ""
            elif op == "FEED":
                success = (
                    has_animal(before_tile) and not before_tile["fed_today"]
                    and has_animal(after_tile) and after_tile["fed_today"]
                )
                source = before_tile.get("animal", "") if isinstance(before_tile, dict) else ""
            elif op == "CARE":
                success = (
                    has_animal(before_tile) and not before_tile["cared_today"]
                    and has_animal(after_tile) and after_tile["cared_today"]
                )
                source = before_tile.get("animal", "") if isinstance(before_tile, dict) else ""
            elif op == "COLLECT_FERTILIZER":
                success = (
                    has_animal(before_tile) and before_tile["fertilizer_available"]
                    and has_animal(after_tile) and not after_tile["fertilizer_available"]
                )
                source = before_tile.get("animal", "") if isinstance(before_tile, dict) else ""
                quantity = int(success)
            elif op == "PLACE":
                success = before is None and after is not None and after[0] == "animal"
                source = after[1] if success else ""
            elif op == "DIG":
                success = before_tile is not None and after_tile is None
                source = before[1] if before is not None else (
                    before_tile.get("kind", "") if isinstance(before_tile, dict) else ""
                )
            elif op in {"BUILD_COOP", "BUILD_PASTURE"}:
                success = before_tile is None and isinstance(after_tile, dict)
                source = op.removeprefix("BUILD_")
            elif op == "DROP":
                after_total = sum(private["shed"].values()) + sum(
                    private["inventories"][index].values()
                )
                quantity = before_shed + before_inventory - after_total
                success = before_inventory > 0

            if context is None or not success:
                continue
            event = {
                "episode_id": context["episode_id"],
                "team": context["team"],
                "rank": context["rank"],
                "result_index": result_index,
                "day": day,
                "hour": context["hour"],
                "operation": op,
                "source": source,
                "origin_day": before[2] if before is not None else "",
                "x": x,
                "y": y,
                "quantity": quantity,
            }
            if op == "HARVEST" and before[0] == "animal":
                identity = (x, y, source, before[2])
                ready = context["ready_since"].pop(identity, context["step"])
                event["ready_latency"] = context["step"] - ready
                event["collected_all"] = int(
                    has_animal(after_tile) and after_tile.get("yield_units", 0) == 0
                )
            else:
                event["ready_latency"] = ""
                event["collected_all"] = ""
            if op == "PLACE":
                distance = center_distance(x, y)
                closer_nonanimal = 0
                closer_buildable = 0
                closer_ready = 0
                for cy, row in enumerate(farm["tiles"]):
                    for cx, tile in enumerate(row):
                        if tile == "LOCKED" or center_distance(cx, cy) >= distance:
                            continue
                        if not has_animal(tile):
                            closer_nonanimal += 1
                        if tile is None or (
                            isinstance(tile, dict)
                            and tile.get("kind") == STRUCTURE[source]
                            and not tile.get("animal")
                        ):
                            closer_buildable += 1
                        if (
                            isinstance(tile, dict)
                            and tile.get("kind") == STRUCTURE[source]
                            and not tile.get("animal")
                        ):
                            closer_ready += 1
                event["center_distance"] = distance
                event["closer_nonanimal_tiles"] = closer_nonanimal
                event["closer_buildable_sites"] = closer_buildable
                event["closer_ready_structures"] = closer_ready
            else:
                event["center_distance"] = ""
                event["closer_nonanimal_tiles"] = ""
                event["closer_buildable_sites"] = ""
                event["closer_ready_structures"] = ""
            unit_events.append(event)
            if op in {"FEED", "CARE", "COLLECT_FERTILIZER"}:
                key = (x, y, source, before[2], day)
                context["animal_days"][key][op] = True
            if op in {"WATER", "FERTILIZE"} and before is not None:
                key = (x, y, source, before[2], day)
                context["crop_days"][key][op] = True
            if op == "DROP" and quantity > 0:
                context["drop_discards"] += quantity


def process_market(farms, privates, market, actions, contexts, transactions):
    queues = [action.get("market", [])[:10] for action in actions]
    max_len = max(map(len, queues), default=0)
    for order_index in range(max_len):
        states = []
        events = []
        for player, queue in enumerate(queues):
            parsed = game._parse_order(queue[order_index]) if order_index < len(queue) else None
            states.append(parsed)
            context = contexts.get(player)
            event = None
            if parsed is not None and context is not None:
                event = {
                    "episode_id": context["episode_id"],
                    "team": context["team"],
                    "rank": context["rank"],
                    "day": context["day"],
                    "hour": context["hour"],
                    "operation": parsed["type"],
                    "item": parsed.get("item", ""),
                    "requested": parsed.get("remaining", 1),
                    "actual": 0,
                    "value": 0,
                    "shop_demand": context["demand"].get(parsed.get("item", ""), 0),
                }
                transactions.append(event)
            events.append(event)

        for player, state in enumerate(states):
            if state is None:
                continue
            before_money = farms[player]["money"]
            if state["type"] == "HIRE":
                game._do_hire(farms[player], privates[player], 10, 1)
                states[player] = None
            elif state["type"] == "BUY_LAND":
                game._do_buy_land(farms[player], 10)
                states[player] = None
            if events[player] is not None and before_money != farms[player]["money"]:
                events[player]["actual"] = 1
                events[player]["value"] = abs(farms[player]["money"] - before_money)

        while True:
            quoted = [None, None]
            for player, state in enumerate(states):
                if state is None or state["remaining"] <= 0:
                    continue
                op = state["type"]
                item = state["item"]
                if op == "SELL" and item in game.PRODUCTS:
                    price = game.market_price(item, market["inventory"][item], market.get("params"))
                elif op == "BUY_PRODUCT" and item in ("WHEAT", "FERTILIZER"):
                    price = game.market_price(item, market["inventory"][item] - 1, market.get("params"))
                elif op == "BUY_SEED" and item in game.CROPS:
                    price = game.CROPS[item]["seed"]
                elif op == "BUY_ANIMAL" and item in game.ANIMALS:
                    price = game.ANIMALS[item]["cost"]
                else:
                    states[player] = None
                    continue
                quoted[player] = (op, item, price, state)
            if all(value is None for value in quoted):
                break
            committed = False
            for player, quote in enumerate(quoted):
                if quote is None:
                    continue
                op, item, price, state = quote
                if game._commit_unit(op, item, price, farms[player], privates[player], market, 100):
                    state["remaining"] -= 1
                    committed = True
                    if events[player] is not None:
                        events[player]["actual"] += 1
                        events[player]["value"] += price
                else:
                    states[player] = None
            if not committed:
                break
        game._refresh_prices(market)


def record_observations(replay, seat, context):
    shed_values = []
    active_at_day_start = defaultdict(Counter)
    for state_index, entries in enumerate(replay["steps"]):
        observation = entries[seat]["observation"]
        farm = observation["farms"][seat]
        private = observation["private"]
        day = observation["day"]
        shed_values.append(sum(private["shed"].values()))
        demand = shop_demand(observation["town"]["unlocked_shops"])
        context["day_demand"].setdefault(day, demand)
        for y, row in enumerate(farm["tiles"]):
            for x, tile in enumerate(row):
                if is_crop(tile):
                    crop = tile["crop"]
                    planted = tile["planted_day"]
                    key = (x, y, crop, planted, day)
                    values = context["crop_days"][key]
                    values["present"] = True
                    values["WATER"] |= tile["watered_today"]
                    values["fertilizer_active"] |= tile["fertilized_until_day"] >= day
                    age = day - planted
                    values["yield_relevant"] = age in BONUS_AGES[crop]
                    if observation["hour"] == 0:
                        active_at_day_start[day][crop] += 1
                elif has_animal(tile):
                    animal = tile["animal"]
                    placed = tile["placed_day"]
                    key = (x, y, animal, placed, day)
                    values = context["animal_days"][key]
                    values["present"] = True
                    values["FEED"] |= tile["fed_today"]
                    values["CARE"] |= tile["cared_today"]
                    if tile.get("yield_units", 0) > 0:
                        context["yield_observations"][animal] += 1
                        if tile["yield_units"] >= ANIMAL_CAP[animal]:
                            context["cap_observations"][animal] += 1
                    if observation["hour"] == 0:
                        active_at_day_start[day][animal] += 1
    context["shed_values"] = shed_values
    context["active_at_day_start"] = active_at_day_start


def record_instances(replay, seat, context, unit_events, crop_instances,
                     animal_instances, transitions, tile_rows):
    removal = {}
    for event in unit_events:
        if (
            event["episode_id"] == context["episode_id"]
            and event["team"] == context["team"]
            and event["operation"] in {"HARVEST", "DIG"}
        ):
            removal[(event["result_index"], event["x"], event["y"], event["source"])] = event["operation"]
    sequences = defaultdict(list)
    active = {}
    steps = replay["steps"]
    for state_index, entries in enumerate(steps):
        farm = entries[seat]["observation"]["farms"][seat]
        for y, row in enumerate(farm["tiles"]):
            for x, tile in enumerate(row):
                identity = occupant(tile)
                prior = active.get((x, y))
                if identity == (prior["kind"], prior["name"], prior["origin_day"]) if prior else identity is None:
                    continue
                if prior is not None:
                    close_instance(
                        prior, state_index, False, steps, seat, tile, removal,
                        crop_instances, animal_instances,
                    )
                    sequences[(x, y)].append(prior)
                    del active[(x, y)]
                if identity is not None:
                    active[(x, y)] = {
                        "episode_id": context["episode_id"],
                        "team": context["team"],
                        "rank": context["rank"],
                        "x": x,
                        "y": y,
                        "kind": identity[0],
                        "name": identity[1],
                        "origin_day": identity[2],
                        "start_state": state_index,
                    }
    for coordinate, instance in active.items():
        close_instance(
            instance, len(steps) - 1, True, steps, seat,
            tile_at(steps[-1][seat]["observation"]["farms"][seat], *coordinate),
            removal, crop_instances, animal_instances,
        )
        sequences[coordinate].append(instance)

    for (x, y), sequence in sequences.items():
        broad_switches = 0
        specific_changes = 0
        for before, after in zip(sequence, sequence[1:]):
            broad_switches += before["kind"] != after["kind"]
            specific_changes += before["name"] != after["name"]
            transitions.append({
                "episode_id": context["episode_id"],
                "team": context["team"],
                "rank": context["rank"],
                "x": x,
                "y": y,
                "from_kind": before["kind"],
                "from_name": before["name"],
                "to_kind": after["kind"],
                "to_name": after["name"],
                "gap_turns": after["start_state"] - before["end_state"],
            })
        tile_rows.append({
            "episode_id": context["episode_id"],
            "team": context["team"],
            "rank": context["rank"],
            "x": x,
            "y": y,
            "instances": len(sequence),
            "transitions": max(0, len(sequence) - 1),
            "broad_switches": broad_switches,
            "specific_changes": specific_changes,
            "ever_crop": int(any(value["kind"] == "crop" for value in sequence)),
            "ever_animal": int(any(value["kind"] == "animal" for value in sequence)),
        })


def close_instance(instance, end_state, censored, steps, seat, next_tile, removal,
                   crop_instances, animal_instances):
    instance["end_state"] = end_state
    instance["censored"] = int(censored)
    previous = steps[max(0, end_state - 1)][seat]["observation"]
    instance["end_day"] = previous["day"]
    instance["end_hour"] = previous["hour"]
    instance["age_days"] = previous["day"] - instance["origin_day"]
    if censored:
        instance["outcome"] = "terminal"
    elif isinstance(next_tile, dict) and next_tile.get("kind") == "WEED":
        instance["outcome"] = "weed"
    else:
        instance["outcome"] = removal.get(
            (end_state, instance["x"], instance["y"], instance["name"]),
            "escape" if instance["kind"] == "animal" else "other",
        ).lower()
    target = crop_instances if instance["kind"] == "crop" else animal_instances
    target.append(dict(instance))


def main():
    global WORK,MANIFEST
    parser=argparse.ArgumentParser()
    parser.add_argument("--research-dir",type=Path,default=WORK)
    args=parser.parse_args()
    WORK=args.research_dir.resolve()
    assert WORK.is_relative_to(ROOT)
    MANIFEST=WORK/"top_replay_manifest.csv"
    with MANIFEST.open(newline="") as source:
        manifest = list(csv.DictReader(source))
    targets = defaultdict(dict)
    metadata = {}
    for row in manifest:
        episode = int(row["episode_id"])
        targets[episode][row["team"]] = row
        metadata[row["team"]] = row

    transactions = []
    unit_events = []
    crop_instances = []
    animal_instances = []
    transitions = []
    tile_rows = []
    animal_days = []
    crop_days = []
    game_rows = []
    daily_rows = []
    fingerprints = []
    validations = []

    for file_index, (episode, team_rows) in enumerate(sorted(targets.items()), 1):
        path = REPLAYS / f"episode-{episode}-replay.json"
        replay = json.loads(path.read_bytes())
        names = replay["info"]["TeamNames"]
        seat_contexts = {}
        replay_unit_start = len(unit_events)
        for team, row in team_rows.items():
            replay_team = row.get("replay_team", team)
            if names.count(replay_team) != 1:
                raise RuntimeError(f"cannot find {team} in episode {episode}: {names}")
            seat = names.index(replay_team)
            context = {
                "episode_id": episode,
                "team": team,
                "rank": int(row["rank"]),
                "seat": seat,
                "ready_since": {},
                "animal_days": defaultdict(lambda: defaultdict(bool)),
                "crop_days": defaultdict(lambda: defaultdict(bool)),
                "yield_observations": Counter(),
                "cap_observations": Counter(),
                "day_demand": {},
                "drop_discards": 0,
                "dayend_discards": 0,
                "dayend_overflow_events": 0,
                "unit_requests": 0,
                "unit_faults": Counter(),
            }
            record_observations(replay, seat, context)
            seat_contexts[seat] = context

        steps = replay["steps"]
        action_streams = {seat: [] for seat in seat_contexts}
        inventory_orders = [[list(inventory) for inventory in steps[0][seat]["observation"]["private"]["inventories"]] for seat in range(2)]
        for result_index in range(1, len(steps)):
            previous = steps[result_index - 1]
            current = steps[result_index]
            public = previous[0]["observation"]
            day = public["day"]
            hour = public["hour"]
            actions = [entry.get("action") or {} for entry in current]
            farms = copy.deepcopy(public["farms"])
            privates = [copy.deepcopy(previous[player]["observation"]["private"]) for player in range(2)]
            # Replay JSON sorts dictionary keys. DROP overflow depends on the
            # original insertion order, so carry that order from earlier actions.
            for player in range(2):
                for unit, inventory in enumerate(privates[player]["inventories"]):
                    order = inventory_orders[player][unit] if unit < len(inventory_orders[player]) else []
                    keys = [key for key in order if key in inventory]
                    keys.extend(key for key in inventory if key not in keys)
                    privates[player]["inventories"][unit] = {key: inventory[key] for key in keys}
            market = copy.deepcopy(public["market"])
            contexts = {}
            for seat, context in seat_contexts.items():
                context["day"] = day
                context["hour"] = hour
                context["step"] = public["step"]
                context["demand"] = shop_demand(public["town"]["unlocked_shops"])
                contexts[seat] = context
                action_streams[seat].append(actions[seat])
                live_yield = set()
                for y, row in enumerate(farms[seat]["tiles"]):
                    for x, tile in enumerate(row):
                        if not has_animal(tile):
                            continue
                        identity = (x, y, tile["animal"], tile["placed_day"])
                        if tile.get("yield_units", 0) > 0:
                            live_yield.add(identity)
                            context["ready_since"].setdefault(identity, public["step"])
                for identity in set(context["ready_since"]) - live_yield:
                    del context["ready_since"][identity]
            apply_units(farms, privates, actions, day, result_index, contexts, unit_events)
            process_market(farms, privates, market, actions, contexts, transactions)
            inventory_orders = [[list(inventory) for inventory in private["inventories"]] for private in privates]
            if hour == 23:
                inventory_orders = [[[]], [[]]]
            for seat, context in seat_contexts.items():
                observed_money = current[seat]["observation"]["farms"][seat]["money"]
                validations.append(abs(farms[seat]["money"] - observed_money))
                if validations[-1] != 0:
                    witness = {
                        "episode": episode, "seat": seat, "day": day, "hour": hour,
                        "expected_money": observed_money, "reconstructed_money": farms[seat]["money"],
                        "actions": actions, "configuration": replay.get("configuration"),
                        "before": previous, "after": current,
                    }
                    (WORK / "replay_mismatch.json").write_text(json.dumps(witness, indent=2))
                    raise RuntimeError(f"money mismatch episode={episode} seat={seat} d={day} h={hour}: {validations[-1]}")
                if hour == 23:
                    overflow = max(0, portable_total(privates[seat]) - 100)
                    context["dayend_discards"] += overflow
                    context["dayend_overflow_events"] += overflow > 0

        replay_unit_events = unit_events[replay_unit_start:]
        for seat, context in seat_contexts.items():
            stream = action_streams[seat]
            day0 = stream[:24]
            semantic = []
            for action in day0:
                units = [action.get("farmer", ["PASS"]), *action.get("hands", [])]
                semantic.append({
                    "units": [command[:2] for command in units if command and command[0] != "PASS"],
                    "market": [command[:2] for command in action.get("market", [])],
                })
            fingerprints.append({
                "episode_id": episode,
                "team": context["team"],
                "rank": context["rank"],
                "seat": seat,
                "day0_exact_sha256": hashlib.sha256(json.dumps(
                    day0, sort_keys=True, separators=(",", ":")
                ).encode()).hexdigest(),
                "day0_semantic_sha256": hashlib.sha256(json.dumps(
                    semantic, sort_keys=True, separators=(",", ":")
                ).encode()).hexdigest(),
                "full_exact_sha256": hashlib.sha256(json.dumps(
                    stream, sort_keys=True, separators=(",", ":")
                ).encode()).hexdigest(),
            })
            record_instances(
                replay, seat, context, replay_unit_events, crop_instances,
                animal_instances, transitions, tile_rows,
            )

            for (x, y, animal, placed, day), values in context["animal_days"].items():
                animal_days.append({
                    "episode_id": episode,
                    "team": context["team"],
                    "rank": context["rank"],
                    "animal": animal,
                    "placed_day": placed,
                    "day": day,
                    "fed": int(values["FEED"]),
                    "cared": int(values["CARE"]),
                    "collected_fertilizer": int(values["COLLECT_FERTILIZER"]),
                })
            for (x, y, crop, planted, day), values in context["crop_days"].items():
                crop_days.append({
                    "episode_id": episode,
                    "team": context["team"],
                    "rank": context["rank"],
                    "crop": crop,
                    "planted_day": planted,
                    "day": day,
                    "watered": int(values["WATER"]),
                    "fertilized_active": int(values["fertilizer_active"] or values["FERTILIZE"]),
                    "yield_relevant": int(values["yield_relevant"]),
                    "maximized": int(
                        values["yield_relevant"] and values["WATER"]
                        and (values["fertilizer_active"] or values["FERTILIZE"])
                    ),
                })

            final = steps[-1][seat]["observation"]
            pending = Counter()
            for row in final["farms"][seat]["tiles"]:
                for tile in row:
                    if has_animal(tile):
                        pending[tile["animal"]] += tile.get("yield_units", 0)
            shed_values = context["shed_values"]
            game_row = {
                "episode_id": episode,
                "team": context["team"],
                "rank": context["rank"],
                "seat": seat,
                "reward": replay["rewards"][seat],
                "opponent": names[1 - seat],
                "opponent_reward": replay["rewards"][1 - seat],
                "shed_max": max(shed_values),
                "shed_mean": sum(shed_values) / len(shed_values),
                "shed_turns_ge_80": sum(value >= 80 for value in shed_values),
                "shed_turns_ge_90": sum(value >= 90 for value in shed_values),
                "shed_turns_ge_95": sum(value >= 95 for value in shed_values),
                "shed_turns_eq_100": sum(value == 100 for value in shed_values),
                "drop_discards": context["drop_discards"],
                "dayend_discards": context["dayend_discards"],
                "dayend_overflow_events": context["dayend_overflow_events"],
                "unit_requests": context["unit_requests"],
                "unit_faults": sum(context["unit_faults"].values()),
                "unit_faults_by_operation": json.dumps(dict(context["unit_faults"]), sort_keys=True),
            }
            for animal in ANIMALS:
                game_row[f"pending_{ANIMAL_PRODUCT[animal]}"] = pending[animal]
                game_row[f"yield_observations_{animal}"] = context["yield_observations"][animal]
                game_row[f"cap_observations_{animal}"] = context["cap_observations"][animal]
            game_rows.append(game_row)

            game_transactions = [
                event for event in transactions
                if event["episode_id"] == episode and event["team"] == context["team"]
            ]
            game_units = [
                event for event in replay_unit_events
                if event["team"] == context["team"]
            ]
            for day in range(30):
                demand = context["day_demand"].get(day, Counter())
                active = context["active_at_day_start"].get(day, Counter())
                for item in PRODUCTS:
                    daily_rows.append({
                        "episode_id": episode,
                        "team": context["team"],
                        "rank": context["rank"],
                        "day": day,
                        "item": item,
                        "shop_demand": demand[item],
                        "active_tiles": active[item] if item in game.CROPS else active[
                            {value: key for key, value in ANIMAL_PRODUCT.items()}.get(item, "")
                        ],
                        "plants": sum(
                            event["operation"] == "PLANT" and event["source"] == item
                            and event["day"] == day for event in game_units
                        ),
                        "animal_buys": sum(
                            event["operation"] == "BUY_ANIMAL"
                            and ANIMAL_PRODUCT.get(event["item"]) == item
                            and event["day"] == day for event in game_transactions
                            for _ in range(event["actual"])
                        ),
                        "sales": sum(
                            event["actual"] for event in game_transactions
                            if event["operation"] == "SELL" and event["item"] == item
                            and event["day"] == day
                        ),
                    })

        if file_index % 12 == 0 or file_index == len(targets):
            print(f"analyzed {file_index}/{len(targets)} replays", flush=True)

    if max(validations, default=0) != 0:
        raise RuntimeError(f"market/unit simulation money mismatch: {max(validations)}")
    write_csv("transactions.csv", transactions)
    write_csv("unit_events.csv", unit_events)
    write_csv("crop_instances.csv", crop_instances)
    write_csv("animal_instances.csv", animal_instances)
    write_csv("tile_transitions.csv", transitions)
    write_csv("tile_summary.csv", tile_rows)
    write_csv("animal_days.csv", animal_days)
    write_csv("crop_days.csv", crop_days)
    write_csv("game_summary.csv", game_rows)
    write_csv("daily_product.csv", daily_rows)
    write_csv("fingerprints.csv", fingerprints)
    print(
        f"wrote {len(game_rows)} player-games, {len(transactions)} transactions, "
        f"{len(crop_instances)} crop instances, {len(animal_instances)} animal instances"
    )


if __name__ == "__main__":
    main()
