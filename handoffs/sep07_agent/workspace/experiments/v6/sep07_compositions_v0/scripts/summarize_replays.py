import json
from collections import defaultdict
from pathlib import Path

import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "research"
PRODUCTS = (
    "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK",
    "WOOL", "FERTILIZER",
)
CROPS = PRODUCTS[:5]
ANIMALS = ("GOOSE", "COW", "SHEEP")
ANIMAL_PRODUCT = {"GOOSE": "EGG", "COW": "MILK", "SHEEP": "WOOL"}
CAPS = {"GOOSE": 4, "COW": 6, "SHEEP": 6}


def scalar(value):
    if pd.isna(value):
        return None
    if isinstance(value, (np.integer, np.floating)):
        return value.item()
    return value


def quantile(series, value):
    return scalar(series.quantile(value)) if len(series) else None


def add_winner(frame, games):
    winners = games[["episode_id", "team", "won"]]
    return frame.merge(winners, on=["episode_id", "team"], how="left", validate="many_to_one")


def time_profile(frame, weight="actual"):
    frame = frame[frame[weight] > 0]
    total = frame[weight].sum()
    if not total:
        return {"units": 0}
    hours = frame.groupby("hour")[weight].sum().reindex(range(24), fill_value=0)
    days = frame.groupby("day")[weight].sum().reindex(range(30), fill_value=0)
    top_hours = hours.sort_values(ascending=False).head(6)
    top_days = days.sort_values(ascending=False).head(6)
    return {
        "units": int(total),
        "mean_day": float(np.average(frame["day"], weights=frame[weight])),
        "mean_hour": float(np.average(frame["hour"], weights=frame[weight])),
        "hour_share": {str(hour): float(value / total) for hour, value in hours.items()},
        "top_hours": {str(hour): float(value / total) for hour, value in top_hours.items()},
        "hour_mod4_share": {
            str(remainder): float(
                frame.loc[frame.hour % 4 == remainder, weight].sum() / total
            ) for remainder in range(4)
        },
        "day_share": {str(day): float(value / total) for day, value in days.items()},
        "top_days": {str(day): float(value / total) for day, value in top_days.items()},
        "day_band_share": {
            "0-2": float(frame.loc[frame.day <= 2, weight].sum() / total),
            "3-9": float(frame.loc[frame.day.between(3, 9), weight].sum() / total),
            "10-19": float(frame.loc[frame.day.between(10, 19), weight].sum() / total),
            "20-26": float(frame.loc[frame.day.between(20, 26), weight].sum() / total),
            "27-29": float(frame.loc[frame.day >= 27, weight].sum() / total),
        },
    }


def transaction_summary(transactions, games, winning):
    frame = transactions[transactions.won] if winning else transactions
    game_count = int(games.won.sum()) if winning else len(games)
    game_keys = games.loc[games.won] if winning else games
    teams = sorted(game_keys.team.unique())
    output = {}
    operation_items = {
        "SELL": PRODUCTS,
        "BUY_PRODUCT": ("WHEAT", "FERTILIZER"),
        "BUY_SEED": CROPS,
        "BUY_ANIMAL": ANIMALS,
    }
    for operation, items in operation_items.items():
        output[operation] = {}
        operation_frame = frame[frame.operation == operation]
        for item in items:
            values = operation_frame[operation_frame.item == item]
            actual = int(values.actual.sum())
            requested = int(values.requested.sum())
            positive = values.groupby(["episode_id", "team"]).actual.sum()
            by_team = {}
            for team in teams:
                team_games = game_keys[game_keys.team == team]
                team_values = values[values.team == team]
                team_positive = team_values.groupby(["episode_id", "team"]).actual.sum()
                by_team[team] = {
                    "games": len(team_games),
                    "units_per_game": float(team_values.actual.sum() / len(team_games)),
                    "positive_game_share": float((team_positive > 0).sum() / len(team_games)),
                }
            output[operation][item] = {
                "actual_units": actual,
                "requested_units": requested,
                "execution_rate": actual / requested if requested else None,
                "units_per_game": actual / game_count,
                "positive_game_share": float((positive > 0).sum() / game_count),
                "teams_with_positive_units": sum(
                    values.loc[values.team == team, "actual"].sum() > 0 for team in teams
                ),
                "team_count": len(teams),
                "mean_realized_unit_value": (
                    float(values.value.sum() / actual) if actual else None
                ),
                "timing": time_profile(values),
                "by_team": by_team,
            }
    return output


def animal_harvest_summary(units, games, winning):
    frame = units[(units.operation == "HARVEST") & units.source.isin(ANIMALS)]
    if winning:
        frame = frame[frame.won]
        game_rows = games[games.won]
    else:
        game_rows = games
    output = {}
    for animal in ANIMALS:
        values = frame[frame.source == animal]
        quantity = values.quantity.astype(int)
        latency = values.ready_latency.astype(float)
        pending = game_rows[f"pending_{ANIMAL_PRODUCT[animal]}"]
        output[animal] = {
            "harvest_events": len(values),
            "units": int(quantity.sum()),
            "quantity_distribution": {
                str(key): int(value) for key, value in quantity.value_counts().sort_index().items()
            },
            "full_cap_event_share": float((quantity >= CAPS[animal]).mean()) if len(values) else None,
            "collected_all_event_share": float(values.collected_all.astype(float).mean()) if len(values) else None,
            "ready_latency_median_turns": quantile(latency, 0.5),
            "ready_latency_p90_turns": quantile(latency, 0.9),
            "same_day_event_share": float((latency < 24).mean()) if len(values) else None,
            "within_6_turn_event_share": float((latency <= 6).mean()) if len(values) else None,
            "terminal_pending_units_per_game": float(pending.mean()),
            "games_with_zero_terminal_pending_share": float((pending == 0).mean()),
        }
    return output


def crop_lifecycle_summary(instances, units, winning):
    frame = instances[instances.won] if winning else instances
    harvests = units[
        (units.operation == "HARVEST") & units.source.isin(CROPS)
        & (units.won if winning else True)
    ].copy()
    completed_harvests = frame[frame.outcome == "harvest"].merge(
        harvests[[
            "episode_id", "team", "result_index", "x", "y", "source", "quantity"
        ]],
        left_on=["episode_id", "team", "end_state", "x", "y", "name"],
        right_on=["episode_id", "team", "result_index", "x", "y", "source"],
        how="left",
        validate="one_to_one",
    )
    output = {}
    for crop in CROPS:
        values = frame[frame.name == crop]
        ended = values[~values.censored.astype(bool)]
        crop_harvests = completed_harvests[completed_harvests.name == crop]
        output[crop] = {
            "instances": len(values),
            "terminal_survival_share": float(values.censored.mean()) if len(values) else None,
            "outcomes": {str(key): int(value) for key, value in values.outcome.value_counts().items()},
            "completed_age_median_days": quantile(ended.age_days, 0.5),
            "completed_age_p10_days": quantile(ended.age_days, 0.1),
            "completed_age_p90_days": quantile(ended.age_days, 0.9),
            "terminal_age_median_days": quantile(values.loc[values.censored == 1, "age_days"], 0.5),
            "harvest_age_distribution": {
                str(key): int(value)
                for key, value in crop_harvests.age_days.value_counts().sort_index().items()
            },
            "harvest_age_median_days": quantile(crop_harvests.age_days, 0.5),
            "harvest_quantity_distribution": {
                str(int(key)): int(value)
                for key, value in crop_harvests.quantity.dropna().astype(int).value_counts().sort_index().items()
            },
        }
    return output


def animal_service_summary(days, instances, winning):
    frame = days[days.won] if winning else days
    animal_instances = instances[instances.won] if winning else instances
    output = {}
    for animal in ANIMALS:
        values = frame[frame.animal == animal].copy()
        joint = values.fed.astype(bool) & values.cared.astype(bool)
        lifecycles = animal_instances[animal_instances.name == animal]
        daily = values.groupby("day").agg(
            animal_days=("fed", "size"), fed=("fed", "mean"), cared=("cared", "mean")
        )
        output[animal] = {
            "animal_days": len(values),
            "fed_rate": float(values.fed.mean()) if len(values) else None,
            "cared_rate": float(values.cared.mean()) if len(values) else None,
            "fed_and_cared_rate": float(joint.mean()) if len(values) else None,
            "fertilizer_collection_rate": float(values.collected_fertilizer.mean()) if len(values) else None,
            "daily": {
                str(day): {
                    "animal_days": int(row.animal_days),
                    "fed_rate": float(row.fed),
                    "cared_rate": float(row.cared),
                } for day, row in daily.iterrows()
            },
            "instances": len(lifecycles),
            "terminal_retention_share": float(lifecycles.censored.mean()) if len(lifecycles) else None,
            "escape_share": float((lifecycles.outcome == "escape").mean()) if len(lifecycles) else None,
            "escape_day_distribution": {
                str(key): int(value) for key, value in
                lifecycles.loc[lifecycles.outcome == "escape", "end_day"].value_counts().sort_index().items()
            },
        }
    return output


def crop_service_summary(days, units, winning):
    frame = days[days.won] if winning else days
    fertilizes = units[(units.operation == "FERTILIZE") & (units.won if winning else True)]
    output = {}
    for crop in CROPS:
        values = frame[frame.crop == crop]
        relevant = values[values.yield_relevant == 1]
        actions = fertilizes[fertilizes.source == crop]
        output[crop] = {
            "crop_days": len(values),
            "watered_rate": float(values.watered.mean()) if len(values) else None,
            "fertilizer_active_rate": float(values.fertilized_active.mean()) if len(values) else None,
            "yield_relevant_days": len(relevant),
            "yield_relevant_water_rate": float(relevant.watered.mean()) if len(relevant) else None,
            "yield_relevant_fertilizer_rate": float(relevant.fertilized_active.mean()) if len(relevant) else None,
            "yield_relevant_maximized_rate": float(relevant.maximized.mean()) if len(relevant) else None,
            "fertilize_actions": len(actions),
            "fertilize_timing": time_profile(actions.assign(actual=1)),
        }
    return output


def shed_summary(games, winning):
    frame = games[games.won] if winning else games
    return {
        "games": len(frame),
        "max_shed_median": quantile(frame.shed_max, 0.5),
        "max_shed_p90": quantile(frame.shed_max, 0.9),
        "max_shed_maximum": int(frame.shed_max.max()),
        "games_reaching_80_share": float((frame.shed_max >= 80).mean()),
        "games_reaching_90_share": float((frame.shed_max >= 90).mean()),
        "games_reaching_95_share": float((frame.shed_max >= 95).mean()),
        "games_reaching_100_share": float((frame.shed_max >= 100).mean()),
        "turn_share_ge_90": float(frame.shed_turns_ge_90.sum() / (len(frame) * 720)),
        "drop_discards": int(frame.drop_discards.sum()),
        "dayend_discards": int(frame.dayend_discards.sum()),
        "games_with_any_discard_share": float(
            ((frame.drop_discards + frame.dayend_discards) > 0).mean()
        ),
        "by_team": {
            team: {
                "games": len(values),
                "max_shed_median": quantile(values.shed_max, 0.5),
                "reached_100_share": float((values.shed_max == 100).mean()),
                "discard_units": int(values.drop_discards.sum() + values.dayend_discards.sum()),
            } for team, values in frame.groupby("team")
        },
    }


def tile_summary(tiles, transitions, instances, placements, winning):
    tile_values = tiles[tiles.won] if winning else tiles
    transition_values = transitions[transitions.won] if winning else transitions
    animal_values = instances[instances.won] if winning else instances
    placement_values = placements[placements.won] if winning else placements
    pairs = transition_values.assign(
        pair=transition_values.from_name + " -> " + transition_values.to_name
    ).pair.value_counts()
    broad_pairs = transition_values.assign(
        pair=transition_values.from_kind + " -> " + transition_values.to_kind
    ).pair.value_counts()
    animal_next = transition_values[transition_values.from_kind == "animal"]
    return {
        "used_tiles": len(tile_values),
        "tiles_ever_both_crop_and_animal_share": float(
            ((tile_values.ever_crop == 1) & (tile_values.ever_animal == 1)).mean()
        ),
        "tiles_with_no_crop_animal_switch_share": float((tile_values.broad_switches == 0).mean()),
        "broad_switch_distribution": {
            str(key): int(value) for key, value in tile_values.broad_switches.value_counts().sort_index().items()
        },
        "specific_change_distribution": {
            str(key): int(value) for key, value in tile_values.specific_changes.value_counts().sort_index().items()
        },
        "productive_instance_transition_distribution": {
            str(key): int(value) for key, value in tile_values.transitions.value_counts().sort_index().items()
        },
        "transition_count": len(transition_values),
        "top_specific_transitions": {str(key): int(value) for key, value in pairs.head(25).items()},
        "broad_transitions": {str(key): int(value) for key, value in broad_pairs.items()},
        "after_animal_transition_count": len(animal_next),
        "animal_to_animal_count": int((animal_next.to_kind == "animal").sum()),
        "animal_to_different_animal_count": int(
            ((animal_next.to_kind == "animal") & (animal_next.from_name != animal_next.to_name)).sum()
        ),
        "animal_to_crop_count": int((animal_next.to_kind == "crop").sum()),
        "animal_instances": len(animal_values),
        "animal_terminal_share": float(animal_values.censored.mean()) if len(animal_values) else None,
        "placements": len(placement_values),
        "placement_literal_nearest_share": float(
            (placement_values.closer_nonanimal_tiles == 0).mean()
        ) if len(placement_values) else None,
        "placement_no_closer_buildable_site_share": float(
            (placement_values.closer_buildable_sites == 0).mean()
        ) if len(placement_values) else None,
        "placement_no_closer_ready_structure_share": float(
            (placement_values.closer_ready_structures == 0).mean()
        ) if len(placement_values) else None,
        "placement_by_animal": {
            animal: {
                "placements": len(values),
                "literal_nearest_share": float((values.closer_nonanimal_tiles == 0).mean()),
                "no_closer_buildable_site_share": float((values.closer_buildable_sites == 0).mean()),
                "no_closer_ready_structure_share": float((values.closer_ready_structures == 0).mean()),
                "center_distance_distribution": {
                    str(key): int(value) for key, value in
                    values.center_distance.astype(int).value_counts().sort_index().items()
                },
            } for animal, values in placement_values.groupby("source")
        },
        "placement_by_team": {
            team: {
                "placements": len(values),
                "literal_nearest_share": float((values.closer_nonanimal_tiles == 0).mean()),
                "no_closer_buildable_site_share": float((values.closer_buildable_sites == 0).mean()),
                "no_closer_ready_structure_share": float((values.closer_ready_structures == 0).mean()),
            } for team, values in placement_values.groupby("team")
        },
    }


def fe_slope(frame, outcome):
    values = frame[frame.day >= 3].copy()
    if values.shop_demand.nunique() < 2:
        return {"coefficient": None, "incremental_r2": None, "rows": len(values)}
    team = pd.get_dummies(values.team, drop_first=True, dtype=float)
    day = pd.get_dummies(values.day, drop_first=True, dtype=float)
    fixed = np.column_stack([np.ones(len(values)), team.to_numpy(), day.to_numpy()])
    x = np.column_stack([fixed, values.shop_demand.to_numpy(float)])
    y = values[outcome].to_numpy(float)
    base_fit = fixed @ np.linalg.lstsq(fixed, y, rcond=None)[0]
    full_beta = np.linalg.lstsq(x, y, rcond=None)[0]
    full_fit = x @ full_beta
    total = np.sum((y - y.mean()) ** 2)
    base_sse = np.sum((y - base_fit) ** 2)
    full_sse = np.sum((y - full_fit) ** 2)
    return {
        "coefficient": float(full_beta[-1]),
        "incremental_r2": float((base_sse - full_sse) / total) if total else 0.0,
        "rows": len(values),
    }


def shop_summary(daily, winning):
    frame = daily[daily.won] if winning else daily
    output = {}
    for item in PRODUCTS:
        values = frame[frame.item == item]
        output[item] = {
            "active_tiles_per_demand_unit": fe_slope(values, "active_tiles"),
            "plants_per_demand_unit": fe_slope(values, "plants") if item in CROPS else None,
            "animal_buys_per_demand_unit": (
                fe_slope(values, "animal_buys") if item in ANIMAL_PRODUCT.values() else None
            ),
            "sales_per_demand_unit": fe_slope(values, "sales"),
        }
    return output


def fingerprint_summary(fingerprints):
    by_team = {}
    modes = defaultdict(list)
    for team, values in fingerprints.groupby("team"):
        counts = values.day0_semantic_sha256.value_counts()
        mode = counts.index[0]
        by_team[team] = {
            "games": len(values),
            "semantic_day0_variants": len(counts),
            "modal_semantic_day0_share": float(counts.iloc[0] / len(values)),
            "modal_semantic_day0_hash": mode,
            "exact_day0_variants": values.day0_exact_sha256.nunique(),
            "full_stream_variants": values.full_exact_sha256.nunique(),
        }
        modes[mode].append(team)
    return {
        "by_team": by_team,
        "shared_modal_day0_groups": [teams for teams in modes.values() if len(teams) > 1],
        "distinct_modal_day0_openings": len(modes),
    }


def main():
    games = pd.read_csv(WORK / "game_summary.csv")
    games["won"] = games.reward > games.opponent_reward
    transactions = add_winner(pd.read_csv(WORK / "transactions.csv"), games)
    units = add_winner(pd.read_csv(WORK / "unit_events.csv"), games)
    crops = add_winner(pd.read_csv(WORK / "crop_instances.csv"), games)
    animals = add_winner(pd.read_csv(WORK / "animal_instances.csv"), games)
    transitions = add_winner(pd.read_csv(WORK / "tile_transitions.csv"), games)
    tiles = add_winner(pd.read_csv(WORK / "tile_summary.csv"), games)
    animal_days = add_winner(pd.read_csv(WORK / "animal_days.csv"), games)
    crop_days = add_winner(pd.read_csv(WORK / "crop_days.csv"), games)
    daily = add_winner(pd.read_csv(WORK / "daily_product.csv"), games)
    fingerprints = pd.read_csv(WORK / "fingerprints.csv")
    placements = units[units.operation == "PLACE"].copy()

    team_results = {}
    for team, values in games.groupby("team"):
        team_results[team] = {
            "rank": int(values["rank"].iloc[0]),
            "games": len(values),
            "wins": int(values.won.sum()),
            "win_rate": float(values.won.mean()),
            "mean_reward": float(values.reward.mean()),
            "mean_margin": float((values.reward - values.opponent_reward).mean()),
        }

    output = {
        "scope": {
            "leaderboard_snapshot": "2026-09-07T00:48:32Z",
            "leaderboard_top_teams": 12,
            "episodes_per_team": 6,
            "unique_replays": games.episode_id.nunique(),
            "player_games": len(games),
            "winning_player_games": int(games.won.sum()),
            "engine_versions": ["1.32.7"],
            "team_results": team_results,
        },
        "fingerprints": fingerprint_summary(fingerprints),
        "all": {
            "transactions": transaction_summary(transactions, games, False),
            "animal_harvest": animal_harvest_summary(units, games, False),
            "crop_lifecycle": crop_lifecycle_summary(crops, units, False),
            "animal_service": animal_service_summary(animal_days, animals, False),
            "crop_service": crop_service_summary(crop_days, units, False),
            "shed": shed_summary(games, False),
            "tiles": tile_summary(tiles, transitions, animals, placements, False),
            "shop_response": shop_summary(daily, False),
        },
        "winners": {
            "transactions": transaction_summary(transactions, games, True),
            "animal_harvest": animal_harvest_summary(units, games, True),
            "crop_lifecycle": crop_lifecycle_summary(crops, units, True),
            "animal_service": animal_service_summary(animal_days, animals, True),
            "crop_service": crop_service_summary(crop_days, units, True),
            "shed": shed_summary(games, True),
            "tiles": tile_summary(tiles, transitions, animals, placements, True),
            "shop_response": shop_summary(daily, True),
        },
    }
    (WORK / "summary.json").write_text(json.dumps(
        output, indent=2, sort_keys=True,
        default=lambda value: value.item(),
    ) + "\n")
    print(WORK / "summary.json")


if __name__ == "__main__":
    main()
