"""Format C++ checked crop-service continuation guards as local agent packages."""
import argparse
import hashlib
import json
import os
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--ids", nargs="+", type=int, required=True)
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    assert source.is_relative_to(EXP / "runs") and output.is_relative_to(EXP / "runs")
    assert not output.exists()
    output.mkdir()
    catalog_path = EXP / "configs/league.json"
    catalog = json.loads(catalog_path.read_text())
    for index in args.ids:
        donor = source / "proposals" / f"{source.name}_{index}"
        assert (donor / "closure_source.json").is_file()
        name = f"{source.name}_{index}_closure"
        package = output / "proposals" / name
        (package / "source").mkdir(parents=True)
        relative = lambda path: os.path.relpath(path, package / "source")
        header = '#pragma once\n#include "' + relative(EXP / "include/guarded_sequence.hpp") + '"\n'
        header += '#include "' + relative(EXP / "candidates/investment_context_guarded_001_best/source/agent.hpp") + '"\n'
        days = sorted((donor / "closure").iterdir(), key=lambda path: int(path.name))
        for day in days:
            header += '#include "' + relative(day / "schedule.hpp") + '"\n'
        header += f'namespace compositions::{name} {{\ninline std::vector<GuardedDay> days(){{std::vector<GuardedDay> result;\n'
        for folder in days:
            values = [list(map(int, line.split())) for line in (folder / "guard.txt").read_text().splitlines()]
            assert len(values) == 103 and all(len(row) == 13 for row in values[1:101])
            day, quadrants = values[0]
            header += f'{{GuardedDay g;g.plan={{{day},{source.name}_{index}_d{day}::schedule()}};g.quadrants={quadrants};\ng.tiles={{{{'
            header += ','.join('{{' + ','.join(map(str, row[1:])) + '}}' for row in values[1:101]) + '}};\n'
            for key, data in (("check", [row[0] for row in values[1:101]]), ("shed", values[101]), ("seeds", values[102])):
                header += f'g.{key}={{{",".join(map(str, data))}}};\n'
            header += 'result.push_back(std::move(g));}\n'
        header += f'return result;}}\nclass Agent:public GuardedSequenceAgent<investment_context_guarded_001_best::Agent>{{public:Agent():GuardedSequenceAgent(days()){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n'
        (package / "source/agent.hpp").write_text(header)
        (package / "source/agent.cpp").write_text('#include "agent.hpp"\n')
        manifest = {"format_version": 1, "name": name, "header": "source/agent.hpp", "type": f"compositions::{name}::Agent",
                    "sources": ["source/agent.cpp", os.path.relpath(EXP / "league/top_replay_library/source/agent.cpp", package)]}
        (package / "agent.json").write_text(json.dumps(manifest, indent=2) + "\n")
        files = [donor / "closure_source.json", donor / "problem.json", *[p for d in days for p in (d / "guard.txt", d / "schedule.hpp")]]
        lineage = {"parent": "candidates/investment_context_guarded_001_best", "proposal": str(donor.relative_to(EXP)),
                   "source_run": str((source / "RUN.json").relative_to(EXP)), "status": "Discovery, wider-league validation pending",
                   "local_changes": "Extra dated fertilization, funded purchase, V30 first-day route, and source worker-route reuse guarded by changed physical states. Original requested trade quantities and timing preserved. Initial day must match before continuation activates.",
                   "files_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
        (package / "IMPORT.json").write_text(json.dumps(lineage, indent=2) + "\n")
        (package / "README.md").write_text(f"# {name}\n\nCurrent reference plus one productive crop fertilization and its checked route continuation. Starts only from its observed physical day contract; other states use the parent. The first day uses V30, subsequent source routes are rebound and tested on the changed crop. Original sale quantities and timing remain. Physical guards do not certify future financing. Sources and borrowed parent components are attributed in IMPORT.json. Wider-league, fresh-seed, and operational checks are pending; no promotion or submission.\n")
        assert name not in catalog
        catalog[name] = str(package.relative_to(ROOT))
        print(name)
    catalog_path.write_text(json.dumps(catalog, indent=2) + "\n")


if __name__ == "__main__":
    main()
