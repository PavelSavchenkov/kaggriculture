"""Index the competition's episodes and the agents that played them.

The unit that matters is the *submission*, not the Kaggle team: a team ships
several agents over the competition and each has its own rating. The episode
service reports, for both sides of every episode, `submissionId`, `teamId`,
`reward`, and the rating before and after (`initialScore` / `updatedScore`), so
one call yields identity and a rating trajectory together.

Episodes are listed per submission, so the graph is crawled rather than
enumerated: every response names the opponents' submissions, which seeds the
next round. That matters because the CLI no longer exposes `team-submissions`
(dropped after 1.6.x) and there is no other way to enumerate agents.

This costs no replay downloads - it is the cheap half. Run it before deciding
which of the 0.85 TB of published episodes are worth pulling.

    kaggle_index.py --seed 56047440 --max-calls 400 --out corpus/index
"""
from __future__ import annotations

import argparse
import base64
import csv
import gzip
import json
import time
import urllib.error
import urllib.request
from collections import deque
from pathlib import Path
from threading import Lock

API = "https://www.kaggle.com/api/i/competitions.EpisodeService"


def _auth() -> str:
    cred = json.loads((Path.home() / ".kaggle/kaggle.json").read_text())
    return base64.b64encode(f"{cred['username']}:{cred['key']}".encode()).decode()


def list_episodes(submission_id: int, auth: str, retries: int = 5) -> list[dict]:
    req = urllib.request.Request(
        f"{API}/ListEpisodes",
        data=json.dumps({"submissionId": int(submission_id)}).encode(),
        headers={"Content-Type": "application/json", "Authorization": f"Basic {auth}",
                 "User-Agent": "kaggle-cli", "Accept-Encoding": "gzip"})
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(req, timeout=120) as r:
                raw = r.read()
                if r.headers.get("Content-Encoding") == "gzip":
                    raw = gzip.decompress(raw)
            return json.loads(raw).get("episodes") or []
        except urllib.error.HTTPError as e:
            if e.code in (429, 500, 502, 503) and attempt + 1 < retries:
                retry_after = e.headers.get("Retry-After") if e.headers else None
                delay = float(retry_after) if retry_after else min(60.0, 5.0 * 2 ** attempt)
                print(f"  API {e.code}; retrying submission {submission_id} "
                      f"in {delay:.0f}s ({attempt + 1}/{retries})", flush=True)
                time.sleep(delay)
                continue
            raise
    return []


def _write_snapshot(out: Path, episodes: dict[int, dict], seen: set[int],
                    queue: deque[int]) -> Path:
    rows = sorted(episodes.values(), key=lambda row: -int(row["episode_id"]))
    if not rows:
        return out / "episode_agents.csv"
    path = out / "episode_agents.csv"
    temporary = path.with_suffix(".csv.tmp")
    with temporary.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    temporary.replace(path)
    state = out / "crawl_state.json"
    state_tmp = state.with_suffix(".json.tmp")
    state_tmp.write_text(json.dumps({"seen_submissions": sorted(seen),
                                     "queue": list(queue)}, indent=2) + "\n")
    state_tmp.replace(state)
    return path


def _resume(out: Path, seeds: list[int]) -> tuple[dict[int, dict], set[int], deque[int]]:
    path, state_path = out / "episode_agents.csv", out / "crawl_state.json"
    if not path.is_file() or not state_path.is_file():
        return {}, set(), deque(dict.fromkeys(seeds))
    with path.open(newline="") as handle:
        episodes = {int(row["episode_id"]): row for row in csv.DictReader(handle)}
    state = json.loads(state_path.read_text())
    seen = {int(value) for value in state.get("seen_submissions", [])}
    queue = deque(int(value) for value in state.get("queue", []))
    queued = set(queue)
    for seed in seeds:
        if seed not in seen and seed not in queued:
            queue.append(seed)
    return episodes, seen, queue


def crawl(seeds: list[int], max_calls: int, out: Path, pause: float,
          resume: bool = False) -> dict:
    auth = _auth()
    out.mkdir(parents=True, exist_ok=True)
    episodes, seen_sub, queue = (_resume(out, seeds) if resume
                                 else ({}, set(), deque(dict.fromkeys(seeds))))
    queued = set(queue)
    calls = 0
    t0 = time.time()
    print(f"  starting: {len(episodes)} episodes, {len(seen_sub)} submissions done, "
          f"{len(queue)} queued", flush=True)
    try:
        while queue and calls < max_calls:
            sub = queue.popleft()
            queued.discard(sub)
            if sub in seen_sub:
                continue
            try:
                eps = list_episodes(sub, auth)
            except urllib.error.HTTPError as exc:
                if exc.code == 429:
                    queue.append(sub)
                    queued.add(sub)
                    _write_snapshot(out, episodes, seen_sub, queue)
                    print(f"  submission {sub}: rate limited after retries; "
                          "saved progress and cooling down 60s", flush=True)
                    time.sleep(60)
                    continue
                print(f"  submission {sub}: HTTPError {exc}", flush=True)
                seen_sub.add(sub)
                continue
            except Exception as exc:                                # noqa: BLE001
                print(f"  submission {sub}: {type(exc).__name__} {exc}", flush=True)
                seen_sub.add(sub)
                continue
            seen_sub.add(sub)
            calls += 1
            for episode in eps:
                if episode.get("state") != "COMPLETED":
                    continue
                agents = episode.get("agents") or []
                if len(agents) != 2:
                    continue
                episodes[int(episode["id"])] = {
                    "episode_id": int(episode["id"]),
                    "create_time": episode.get("createTime", ""),
                    **{f"{key}{seat}": agents[seat].get(key)
                       for seat in (0, 1)
                       for key in ("submissionId", "teamId", "teamName", "reward",
                                   "initialScore", "updatedScore")},
                }
                for agent in agents:
                    candidate = agent.get("submissionId")
                    if candidate:
                        candidate = int(candidate)
                        if candidate not in seen_sub and candidate not in queued:
                            queue.append(candidate)
                            queued.add(candidate)
            if calls == 1 or calls % 10 == 0:
                _write_snapshot(out, episodes, seen_sub, queue)
                print(f"  {calls}/{max_calls} calls, {len(episodes)} episodes, "
                      f"{len(seen_sub)} submissions done, {len(queue)} queued, "
                      f"{time.time()-t0:.0f}s", flush=True)
            if pause:
                time.sleep(pause)
    finally:
        path = _write_snapshot(out, episodes, seen_sub, queue)
    if not episodes:
        raise RuntimeError("Kaggle returned no completed two-player episodes")
    return {"calls": calls, "episodes": len(episodes), "submissions": len(seen_sub),
            "queued": len(queue), "seconds": time.time() - t0, "path": str(path)}


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--seed", type=int, nargs="+", required=True)
    ap.add_argument("--max-calls", type=int, default=200)
    ap.add_argument("--pause", type=float, default=1.0, help="seconds between API calls")
    ap.add_argument("--out", type=Path, default=Path("corpus/index"))
    ap.add_argument("--resume", action="store_true",
                    help="continue the checkpointed queue and episode index")
    a = ap.parse_args()
    info = crawl(a.seed, a.max_calls, a.out, a.pause, a.resume)
    print(f"\n{info['calls']} calls in {info['seconds']:.0f}s -> "
          f"{info['episodes']} episodes, {info['submissions']} submissions, "
          f"{info['queued']} still queued\nwrote {info['path']}")
