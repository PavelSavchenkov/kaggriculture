"""Deterministic parity-coverage agent, not a competitive strategy."""


def agent(obs, config):
    day = obs.day
    hour = obs.hour
    action = {"farmer": ["PASS"], "hands": [], "market": []}

    if day == 0:
        if hour == 0:
            action["farmer"] = ["BUILD_COOP"]
            action["market"] = [
                ["BUY_ANIMAL", "GOOSE", 1],
                ["BUY_PRODUCT", "WHEAT", 12],
                ["HIRE"], ["HIRE"], ["HIRE"], ["HIRE"],
                ["BUY_SEED", "WHEAT", 3],
            ]
        elif hour == 1:
            action["farmer"] = ["PICKUP", "GOOSE", 40000]
            action["hands"] = [["WEST"], ["NORTH"], ["NORTH"], ["WEST"]]
        elif hour == 2:
            action["farmer"] = ["PLACE", "GOOSE", 40000]
            action["hands"] = [["WEST"], ["WEST"], ["WEST"], ["BUILD_PASTURE"]]
        elif hour == 3:
            action["farmer"] = ["PICKUP", "WHEAT", 40000]
            action["hands"] = [["BUILD_COOP"], ["BUILD_PASTURE"], ["DIG"], ["PASS"]]
        elif hour == 4:
            action["farmer"] = ["FEED"]
            action["hands"] = [["PLANT", "WHEAT"]] * 4
        elif hour == 5:
            action["farmer"] = ["CARE"]
            action["hands"] = [["PLACE", "WHEAT", 40000]] * 4
    else:
        schedule = {
            0: ["PICKUP", "WHEAT", 40000],
            1: ["FEED"],
            2: ["CARE"],
            3: ["COLLECT_FERTILIZER"],
            4: ["HARVEST"],
            5: ["DROP"],
        }
        action["farmer"] = schedule.get(hour, ["PASS"])

    return action
