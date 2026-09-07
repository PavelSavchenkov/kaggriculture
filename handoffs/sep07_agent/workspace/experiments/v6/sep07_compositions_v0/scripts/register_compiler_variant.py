"""Register a typed C++ compiler configuration; does not search or run policy."""
import argparse
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser()
    p.add_argument("name")
    p.add_argument("--program",type=int,default=0)
    p.add_argument("--source-service",action="store_true")
    p.add_argument("--reserve-days",type=int,default=0)
    p.add_argument("--flexible-hiring",action="store_true")
    p.add_argument("--greedy-layout",action="store_true")
    p.add_argument("--estimated-support",action="store_true")
    args=p.parse_args()
    assert args.name.replace("_","").isalnum() and args.name.islower()
    settings=[args.program,not args.greedy_layout,not args.estimated_support,args.source_service,args.reserve_days,args.flexible_hiring]
    kind="compositions::greedy::Agent<"+",".join(str(v).lower() for v in settings)+">"
    target=EXP/"candidates"/args.name
    (target/"source").mkdir(parents=True,exist_ok=True)
    (target/"source/agent.hpp").write_text('#pragma once\n#include "../../composition_greedy_v0/source/agent.hpp"\n')
    (target/"source/agent.cpp").write_text('#include "agent.hpp"\n')
    (target/"agent.json").write_text(json.dumps(dict(format_version=1,name=args.name,header="source/agent.hpp",type=kind,
        sources=["source/agent.cpp","../composition_greedy_v0/source/agent.cpp"]),indent=2)+"\n")
    (target/"README.md").write_text("# "+args.name+"\n\nTyped configuration of composition_greedy_v0: `"+kind+"`.\n\n"
        "Program, layout, workforce/land support and optional service masks are from the base package PROGRAM_SOURCES.json. "
        "Reserve 0 preserves the legacy inconsistent expansion reserve; positive days use a common target herd for buying and selling. "
        "Purchases replenish one day; the longer horizon only retains surplus. Flexible hiring removes the arbitrary $60 reserve when hiring. "
        "Local adaptations; no additional external source. Full source history and evaluation evidence are in LINEAGE.md and results/.\n")
    path=EXP/"configs/league.json";catalog=json.loads(path.read_text())
    catalog[args.name]=str(target.relative_to(EXP.parents[2]))
    path.write_text(json.dumps(catalog,indent=2)+"\n")
    print(args.name,kind)


if __name__=="__main__":main()
