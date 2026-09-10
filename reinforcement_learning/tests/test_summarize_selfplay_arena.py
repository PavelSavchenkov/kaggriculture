from __future__ import annotations

import importlib.util
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "tools/summarize_selfplay_arena.py"
SPEC = importlib.util.spec_from_file_location("summarize_selfplay_arena", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def test_descendant_scores_are_oriented_for_both_pair_orders():
    as_challenger = {
        "challenger": "child", "opponent": "parent", "result": "win",
        "seat": 1, "money": [10, 14],
    }
    as_opponent = {
        "challenger": "parent", "opponent": "child", "result": "loss",
        "seat": 0, "money": [8, 13],
    }
    assert MODULE.descendant_result(as_challenger, "child") == (1.0, 4.0)
    assert MODULE.descendant_result(as_opponent, "child") == (1.0, 5.0)


def test_summary_compares_direct_play_and_field_rating():
    records = [
        {"challenger": "child", "opponent": "parent", "result": "win",
         "seat": 0, "money": [20, 10]},
        {"challenger": "child", "opponent": "parent", "result": "loss",
         "seat": 1, "money": [16, 12]},
    ]
    arena = {
        "ratings": {"child": 1520.0, "parent": 1480.0, "field": 1500.0},
        "stats": {"child": [3, 1, 0], "parent": [1, 3, 0], "field": [2, 2, 0]},
        "games": 6, "forfeits": 0, "seconds": 1.0,
        "agents": ["child", "parent", "field"], "game_records": records,
    }
    exports = {"descendants": [{
        "parent_agent": "parent", "descendant_agent": "child",
        "best_update": 10,
    }]}
    row = MODULE.summarize(arena, exports)["descendants"][0]
    assert row["descendant_rank"] == 1
    assert row["rating_delta_vs_parent"] == 40.0
    assert row["direct_score_vs_parent"] == 0.5
    assert row["direct_mean_money_margin_vs_parent"] == 3.0
    assert row["directly_better_than_parent"] is True
