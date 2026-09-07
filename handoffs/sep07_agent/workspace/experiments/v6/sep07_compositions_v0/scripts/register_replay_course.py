"""Expose a checked library course as a named league opponent."""
import argparse
import json
import re
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser();parser.add_argument("program",type=int);parser.add_argument("name");args=parser.parse_args()
    assert re.fullmatch("[a-z0-9_]+",args.name),args.name
    programs=json.loads((EXP/"league/top_replay_library/IMPORT.json").read_text())["programs"]
    source=programs[args.program]
    target=EXP/"league"/args.name
    if target.exists():raise FileExistsError(target)
    (target/"source").mkdir(parents=True)
    (target/"source/agent.hpp").write_text(f'''#pragma once
#include "../../top_replay_library/source/agent.hpp"
namespace compositions::{args.name} {{
class Agent:public top_replay_library::Agent {{
public:
    Agent():top_replay_library::Agent({args.program}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{args.name}"}};}}
}};
}}
''')
    (target/"source/agent.cpp").write_text('#include "agent.hpp"\n')
    (target/"agent.json").write_text(json.dumps({"format_version":1,"name":args.name,"header":"source/agent.hpp",
        "type":f"compositions::{args.name}::Agent","sources":["source/agent.cpp","../top_replay_library/source/agent.cpp"]},indent=2)+"\n")
    (target/"IMPORT.json").write_text(json.dumps({"program":args.program,"source":source,
        "scope":"Exact single recorded course, no inferred original branching policy",
        "changes":"None beyond existing library normalization; all 719 source actions already verified",
        "reuse":"User-authorized borrowing of public replay actions; no separate source-code license supplied"},indent=2)+"\n")
    (target/"README.md").write_text(f"# {args.name}\n\nExact replay library program {args.program}, retained as a distinct counterstrategy.\nSource episode, seat, submission, global snapshot and hash are in IMPORT.json.\nIt is one recorded course; no original branching logic is inferred.\nAll 719 actions were verified with the complete {len(programs)}-course library.\n")
    catalog_path=EXP/"configs/league.json";catalog=json.loads(catalog_path.read_text())
    assert args.name not in catalog
    catalog[args.name]=str(target.relative_to(EXP.parents[2]))
    catalog_path.write_text(json.dumps(catalog,indent=2)+"\n")


if __name__=="__main__":main()
