"""Format explicitly selected C++ schedules and physical start contracts."""
import argparse
import json
import os
import re
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
FIELDS = "crop animal age_days stored_units consecutive_dry_days pending_care_bonus fertilizer_days_remaining watered_today fed_today cared_today fertilizer_available".split()
KINDS = {"empty": 0, "locked": 1, "weed": 2, "coop": 3, "pasture": 4, "crop": 5}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--base", choices=["opening_router_v2", "opening_router_v3"], required=True)
    parser.add_argument("--proposals", type=int, nargs="+", required=True)
    args = parser.parse_args()
    assert re.fullmatch(r"[a-z0-9_]+", args.name)
    folder = EXP / "candidates" / args.name
    assert not folder.exists()
    (folder / "source").mkdir(parents=True)
    header = f'#pragma once\n#include "../../../include/guarded_day.hpp"\n#include "../../{args.base}/source/agent.hpp"\n'
    records = []
    for proposal in args.proposals:
        name = f"reduce_hires_001_{proposal}"
        source = EXP / "runs/reduce_hires_001/proposals" / name
        metadata = json.loads((source / "agent.json").read_text())
        problem = json.loads((source / "problem.json").read_text())
        header += f'#include "{os.path.relpath(source / "source/agent.hpp", folder / "source")}"\n'
        records.append((name, source, problem))
    header += f'namespace compositions::{args.name} {{\ninline std::vector<GuardedDay> days() {{\nstd::vector<GuardedDay> result;\n'
    for name, source, problem in records:
        source_header = (source / "source/agent.hpp").read_text()
        program, day = map(int, re.search(r"DayOverrideAgent\((\d+),(\d+),schedule", source_header).groups())
        assert program == 55 and day not in (0, 18, 29)
        tiles = problem["start"]["managed_tiles"]
        assert len(tiles) == 100
        touched = {w["tile"] for w in problem["tile_work"]}
        keys, check = [], []
        for cell, tile in enumerate(tiles):
            assert (tile["x"], tile["y"]) == (cell % 10, cell // 10)
            state = tile["state"]
            keys.append([KINDS[state["kind"]], *[int(state[f]) for f in FIELDS]])
            check.append(int(cell in touched or state["kind"] in ("crop", "coop", "pasture")))
        quadrants = sum(t["state"]["kind"] != "locked" for t in tiles) // 25
        header += f'{{GuardedDay d;d.plan={{{day},{name}::schedule()}};\nd.tiles={{{{'
        header += ",".join("{{" + ",".join(map(str, k)) + "}}" for k in keys) + "}};\n"
        for field, data in (("check", check), ("shed", problem["start"]["shed"]), ("seeds", problem["start"]["seeds"])):
            header += f'd.{field}={{{",".join(map(str, data))}}};\n'
        header += f'd.quadrants={quadrants};result.push_back(std::move(d));}}\n'
    header += f'return result;}}\nclass Agent:public GuardedOpeningAgent<{args.base}::Agent> {{public:Agent():GuardedOpeningAgent(days()){{}}\n'
    header += f'static kag::agent::AgentInfo info(){{return {{"{args.name}"}};}}}};\n}}\n'
    (folder / "source/agent.hpp").write_text(header)
    (folder / "source/agent.cpp").write_text('#include "agent.hpp"\n')
    manifest = {"format_version": 1, "name": args.name, "header": "source/agent.hpp", "type": f"compositions::{args.name}::Agent",
                "sources": ["source/agent.cpp", "../../league/public_router/source/agent.cpp", "../../league/top_replay_library/source/agent.cpp"]}
    (folder / "agent.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (folder / "IMPORT.json").write_text(json.dumps({"parent": args.base, "components": [str(s.relative_to(EXP)) for _, s, _ in records],
        "changes": "New local physical day-start guard over complete source55/V30 schedules; copied parent components retain their recorded attribution.",
        "scope": "Exact used/occupied tile state, shed, seeds, owned quadrants and reset worker position; no claim that this certifies future financing."}, indent=2) + "\n")
    (folder / "README.md").write_text(f"# {args.name}\n\nParent {args.base} with explicitly selected V30 whole days guarded by their source physical start contracts. "
        "The guard runs at hour0 in the delayed-hire branch. A mismatch leaves the complete parent policy active for that day. "
        "Untouched empty/weed/locked tiles do not constrain the route. No seed, hidden state, future shop or runtime solver. "
        "This physical guard does not certify future cash or market behavior. Exact parent/component lineage is in IMPORT.json. "
        "Experimental comparison; no promotion, source identity inference or performance claim.\n")
    catalog_path = EXP / "configs/league.json"
    catalog = json.loads(catalog_path.read_text())
    catalog[args.name] = str(folder.relative_to(ROOT))
    catalog_path.write_text(json.dumps(catalog, indent=2) + "\n")
    print(folder)


if __name__ == "__main__":
    main()
