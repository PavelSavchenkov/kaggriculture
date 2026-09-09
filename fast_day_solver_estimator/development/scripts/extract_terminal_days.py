"""Extract selected-team terminal contracts with an explicit23-phase horizon."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("traces", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "data" / args.name
    output.mkdir(exist_ok=False)
    manifest = json.loads((args.traces / "MANIFEST.json").read_text())
    jobs = [(game, seat) for game in manifest["games"] if game["status"] == "exported_unverified" for seat in game["seats"]]

    def run(job):
        game, seat = job
        name = f"{game['episode']}_p{seat['seat']}"
        folder = output / name
        command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(REPO / "day_solver/with_runtime.sh"),
                   str(EXP / "build/extract_terminal"), str(EXP / game["trace"]), str(seat["seat"]), str(folder)]
        with (output / (name + ".log")).open("w") as log:
            subprocess.run(command, cwd=REPO, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {"episode": game["episode"], **seat, "report": json.loads((folder / "REPORT.json").read_text()),
                "problem": str((folder / "problem.json").relative_to(EXP)), "witness": str((folder / "physical_source.actions.txt").relative_to(EXP)),
                "replay_sha256": game["replay_sha256"], "command": command}

    with ThreadPoolExecutor(max_workers=2) as pool:
        games = list(pool.map(run, jobs))
    rows = []
    for game in games:
        if game["report"]["status"] != "FEASIBLE_SOURCE":
            continue
        path = EXP / game["problem"]; p = json.loads(path.read_text())
        source = f"terminal/{game['team_id']}/{game['episode']}_p{game['seat']}_d29"
        rows.append({"source": source, "sources": [source], "source_family": f"team_{game['team_id']}", "episode": game["episode"],
                     "seat": game["seat"], "day": 29, "team": game["team"], "active_hours": 23,
                     "problem": game["problem"], "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                     "physical_key": physical_key(p, active_hours=23), "observed_workers": p["worker_count"],
                     "tasks": sum(len(r["actions"]) for r in p["tile_work"]), "active_tiles": len(p["tile_work"]),
                     "witnesses": [{"path": game["witness"], "verified": True, "active_hours": 23,
                                    "sha256": hashlib.sha256((EXP / game["witness"]).read_bytes()).hexdigest()}]})
    (output / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    report = {"utc": datetime.now(timezone.utc).isoformat(), "source_sha256": hashlib.sha256((args.traces / "MANIFEST.json").read_bytes()).hexdigest(),
              "source_binary_sha256": hashlib.sha256((EXP / "build/extract_terminal").read_bytes()).hexdigest(), "games": games,
              "constraint": "All actual worker and market activity must finish by hour22. Phase23 has only PASS. End_shed represents total remaining goods after a virtual transfer, without assigning them terminal value.",
              "cases": len(rows), "excluded": len(games) - len(rows)}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"games": len(games), "accepted": len(rows), "excluded": len(games) - len(rows),
                      "physical_contracts": len({r["physical_key"] for r in rows})}))


if __name__ == "__main__":
    main()
