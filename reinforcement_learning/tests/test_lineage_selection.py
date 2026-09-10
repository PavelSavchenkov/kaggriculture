import csv
import importlib.util
from pathlib import Path


MODULE_PATH = Path("reinforcement_learning/tools/select_lineages.py")
SPEC = importlib.util.spec_from_file_location("select_lineages", MODULE_PATH)
select_lineages = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(select_lineages)


def test_api_csv_uses_submission_identity_and_updated_score(tmp_path):
    path = tmp_path / "episode_agents.csv"
    fields = ["episode_id", "create_time",
              "submissionId0", "teamId0", "reward0", "initialScore0", "updatedScore0",
              "submissionId1", "teamId1", "reward1", "initialScore1", "updatedScore1"]
    with path.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerow({
            "episode_id": 17, "create_time": "2026-09-09T00:00:00Z",
            "submissionId0": 101, "teamId0": 1, "reward0": 4000,
            "initialScore0": 2700, "updatedScore0": 2712,
            "submissionId1": 202, "teamId1": 2, "reward1": 3000,
            "initialScore1": 2500, "updatedScore1": 2488,
        })
    ratings = select_lineages.load_ratings(path)
    assert ratings[17][0]["submission_id"] == 101
    assert ratings[17][0]["score"] == 2712
    assert ratings[17][1]["submission_id"] == 202
    latest = select_lineages.latest_submission_ratings(ratings)
    assert latest[101]["elo"] == 2712


def test_distinct_team_selection_does_not_choose_two_versions():
    entries = [
        {"identity": "submission:1", "team": "A", "elo": 2900,
         "trajectories": [{}] * 50},
        {"identity": "submission:2", "team": "A", "elo": 2850,
         "trajectories": [{}] * 60},
        {"identity": "submission:3", "team": "B", "elo": 2800,
         "trajectories": [{}] * 70},
    ]
    selected = select_lineages.select_ranked(entries, 2, False)
    assert [entry["identity"] for entry in selected] == ["submission:1", "submission:3"]
