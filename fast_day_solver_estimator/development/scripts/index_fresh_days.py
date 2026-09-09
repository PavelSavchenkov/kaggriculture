"""Index verified selected-team days without importing opponent examples."""
import argparse
import hashlib
import json
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("days", type=Path)
    args = parser.parse_args()
    folder = args.days.resolve()
    manifest = json.loads((folder / "MANIFEST.json").read_text())
    records, excluded = [], []
    for game in manifest["games"]:
        for day in game["days"]:
            if day["status"] != "FEASIBLE_SOURCE":
                excluded.append({"episode": game["episode"], "seat": game["seat"], "day": day["day"], "status": day["status"]})
                continue
            path = EXP / day["problem"]
            p = json.loads(path.read_text())
            records.append({"source": f"fresh/{game['team_id']}/{game['episode']}_p{game['seat']}_d{day['day']:02}",
                "source_family": f"team_{game['team_id']}", "episode": game["episode"], "seat": game["seat"], "day": day["day"],
                "team": game["team"], "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                "problem": day["problem"], "physical_key": physical_key(p), "observed_workers": p["worker_count"],
                "tasks": sum(len(w["actions"]) for w in p["tile_work"]), "active_tiles": len(p["tile_work"]),
                "sources": [f"fresh/{game['team_id']}/{game['episode']}_p{game['seat']}_d{day['day']:02}"],
                "witnesses": [{"path": day["source_schedule"], "verified": True,
                               "sha256": hashlib.sha256((EXP / day["source_schedule"]).read_bytes()).hexdigest()}]})
    (folder / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    (folder / "EXCLUDED.json").write_text(json.dumps(excluded, indent=2) + "\n")
    print(json.dumps({"records": len(records), "physical_contracts": len({r["physical_key"] for r in records}),
                      "families": len({r["source_family"] for r in records}), "excluded": len(excluded)}))


if __name__ == "__main__":
    main()
