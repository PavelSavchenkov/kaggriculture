"""Package an explicitly chosen C++ day component over the compiled parent."""
import argparse
import json
import os
import re
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("day", type=int)
    parser.add_argument("proposal", type=int)
    parser.add_argument("--base", default="combine_hires_001_best")
    args = parser.parse_args()
    assert re.fullmatch(r"[a-z0-9_]+", args.name)
    assert re.fullmatch(r"[a-z0-9_]+", args.base)
    base = EXP / "candidates" / args.base
    source_name = f"reduce_hires_001_{args.proposal}"
    source = EXP / "runs/reduce_hires_001/proposals" / source_name
    assert (source / "agent.json").exists()
    folder = EXP / "candidates" / args.name
    assert not folder.exists()
    (folder / "source").mkdir(parents=True)
    includes = [os.path.relpath(p / "source/agent.hpp", folder / "source") for p in (base, source)]
    header = f'''#pragma once
#include "{includes[0]}"
#include "{includes[1]}"
namespace compositions::{args.name} {{
inline std::vector<DayPlan> plans() {{
    auto result={args.base}::plans();
    result.push_back({{{args.day},{source_name}::schedule()}});
    return result;
}}
class Agent:public PlannedOpeningAgent {{
public:
    Agent():PlannedOpeningAgent(plans()) {{}}
    static kag::agent::AgentInfo info() {{return {{"{args.name}"}};}}
}};
}}
'''
    (folder / "source/agent.hpp").write_text(header)
    (folder / "source/agent.cpp").write_text('#include "agent.hpp"\n')
    sources = ["source/agent.cpp", *[os.path.relpath(EXP / "league" / p / "source/agent.cpp", folder) for p in ("public_router", "top_replay_library")]]
    manifest = {"format_version": 1, "name": args.name, "header": "source/agent.hpp",
                "type": f"compositions::{args.name}::Agent", "sources": sources}
    (folder / "agent.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (folder / "README.md").write_text(f"# {args.name}\n\nParent {args.base} plus day {args.day} from {source_name}. "
        "The local V30 compiler removes one hire while preserving source55's service, physical endpoints and other accepted markets. "
        "All earlier v2 replay, finance and sale-component lineage is retained in parent packages. "
        "Selected components improved aggregate discovery margin but some failed the stricter per-opponent nondecrease gate. Their combination needs its own causal comparison. "
        "Retained for broader testing; no promotion or causal generalization from a single endpoint. Required checks pending.\n")
    catalog_path = EXP / "configs/league.json"
    catalog = json.loads(catalog_path.read_text())
    for package in (base, folder):
        catalog[package.name] = str(package.relative_to(ROOT))
    catalog_path.write_text(json.dumps(catalog, indent=2) + "\n")
    print(folder)


if __name__ == "__main__":
    main()
