"""Per-perspective metadata for every trace (data/traces/*.txt).

Kaggriculture submissions and teams are missing from Meta Kaggle's Submissions and Teams
tables while the competition runs, so the join uses:
- Meta Kaggle Episodes: the competition's episodes and their times;
- Meta Kaggle EpisodeAgents: submission id, rating before/after and reward per
  (episode, seat), and for every submission its first/last episode time, episode count
  and latest rating over all competition episodes (first episode ~ submission date);
- Kaggle API listings (data/metadata/*/episodes_*.json, submissions.json): submission ->
  team id and name, submission date and public score where available;
- team names recorded with the traces (daily/local perspectives, corpus team ids);
- the public leaderboard csv (data/leaderboard): team id by name, rank and score.
Writes data/perspectives_meta.csv, one row per (episode, seat).
"""
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
META = ROOT / "data/meta_kaggle"
csv.field_size_limit(sys.maxsize)


def rows(name):
    with (META / name).open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        yield {k: i for i, k in enumerate(header)}
        yield from reader


def main():
    episodes = {int(p.stem) for p in (ROOT / "data/traces").glob("*.txt")}
    # Competition id from our episodes, then every episode of the competition.
    stream = rows("Episodes.csv")
    col = next(stream)
    competition = None
    for r in stream:
        if int(r[col["Id"]]) in episodes:
            competition = r[col["CompetitionId"]]
            break
    times = {}
    stream = rows("Episodes.csv")
    col = next(stream)
    for r in stream:
        if r[col["CompetitionId"]] == competition:
            times[int(r[col["Id"]])] = r[col["CreateTime"]]
    print(f"competition {competition}: {len(times)} episodes", flush=True)

    def when(episode):  # sortable "YYYY-MM-DD HH:MM:SS" from "MM/DD/YYYY HH:MM:SS"
        t = times.get(episode, "")
        return f"{t[6:10]}-{t[0:2]}-{t[3:5]} {t[11:]}" if t else ""

    agents, stats = {}, {}
    stream = rows("EpisodeAgents.csv")
    col = next(stream)
    for r in stream:
        episode = int(r[col["EpisodeId"]])
        if episode not in times or not r[col["SubmissionId"]]:
            continue
        sub = int(float(r[col["SubmissionId"]]))
        t = when(episode)
        s = stats.setdefault(sub, {"first": t, "last": t, "episodes": 0, "latest": r[col["UpdatedScore"]]})
        s["episodes"] += 1
        s["first"] = min(s["first"], t)
        if t >= s["last"]:
            s["last"], s["latest"] = t, r[col["UpdatedScore"]]
        if episode in episodes:
            agents.setdefault(episode, {})[int(r[col["Index"]])] = (sub, r[col["InitialScore"]], r[col["UpdatedScore"]],
                                                                    float(r[col["Reward"]] or 0))
    print(f"submissions {len(stats)}; our episodes with agents {len(agents)}", flush=True)

    # Submission -> team from API listings; submission dates and public scores.
    team_of, name_of_team, dates, scores = {}, {}, {}, {}
    for path in (ROOT / "data/metadata").glob("*/episodes_*.json"):
        for e in json.loads(path.read_text()):
            for a in e["agents"]:
                team_of[a["submissionId"]] = a["teamId"]
                name_of_team[a["teamId"]] = a["teamName"]
    for path in (ROOT / "data/metadata").glob("*/submissions.json"):
        for s in json.loads(path.read_text()):
            team_of.setdefault(s["id"], int(path.parent.name))
            dates[s["id"]], scores[s["id"]] = s["dateSubmitted"], s.get("publicScore", "")
    board = next((ROOT / "data/leaderboard").glob("*.csv"))
    with board.open(encoding="utf-8-sig") as f:
        leaderboard = {int(r["TeamId"]): r for r in csv.DictReader(f)}
    id_by_name = {r["TeamName"]: team for team, r in leaderboard.items()}
    for team, r in leaderboard.items():
        name_of_team.setdefault(team, r["TeamName"])
    # Team names recorded with the traces (fill submissions the listings do not cover).
    names = {}
    for source in ("daily_perspectives.csv", "local_perspectives.csv"):
        with (ROOT / "data" / source).open() as f:
            for r in csv.DictReader(f):
                names[(int(r["episode"]), int(r["seat"]))] = r["team"]
    with (ROOT / "data/corpus.txt").open() as f:
        for line in f:
            episode, seat, team = map(int, line.split()[:3])
            if team in name_of_team:
                names.setdefault((episode, seat), name_of_team[team])
    for (episode, seat), name in names.items():
        sub = agents.get(episode, {}).get(seat, (0,))[0]
        if sub and sub not in team_of and name in id_by_name:
            team_of[sub] = id_by_name[name]

    out = ROOT / "data/perspectives_meta.csv"
    written = with_team = 0
    with out.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["episode", "seat", "episode_time", "team_id", "team_name", "submission_id", "submission_date",
                         "submission_first_episode", "submission_last_episode", "submission_episodes",
                         "submission_latest_rating", "submission_public_score", "rating_before", "rating_after", "reward",
                         "opponent_team_id", "opponent_team_name", "opponent_submission_id", "opponent_rating_before",
                         "margin", "team_rank", "team_score"])
        for episode in sorted(agents):
            seats = agents[episode]
            if set(seats) != {0, 1}:
                continue
            for seat in (0, 1):
                (sub, before, after, reward), (other, other_before, _, other_reward) = seats[seat], seats[1 - seat]
                team, other_team = team_of.get(sub, 0), team_of.get(other, 0)
                name = name_of_team.get(team) or names.get((episode, seat), "")
                s = stats.get(sub, {})
                lb = leaderboard.get(team, {})
                writer.writerow([episode, seat, when(episode), team, name, sub, dates.get(sub, ""), s.get("first", ""),
                                 s.get("last", ""), s.get("episodes", 0), s.get("latest", ""), scores.get(sub, ""), before,
                                 after, reward, other_team, name_of_team.get(other_team) or names.get((episode, 1 - seat), ""),
                                 other, other_before, reward - other_reward, lb.get("Rank", ""), lb.get("Score", "")])
                written += 1
                with_team += team != 0
    print(f"wrote {written} perspective rows ({with_team} with team id) to {out}")


if __name__ == "__main__":
    main()
