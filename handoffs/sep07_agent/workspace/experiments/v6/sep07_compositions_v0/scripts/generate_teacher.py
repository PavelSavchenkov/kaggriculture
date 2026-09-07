"""Format a selected recorded policy trace for a C++ execution reference."""
import hashlib
import json
from pathlib import Path

from generate_public_routes import triple

EXP=Path(__file__).resolve().parents[1]
TARGET=EXP/"league/leader_program0_tape"


def main():
    source=json.loads((EXP/"candidates/composition_greedy_v0/PROGRAM_SOURCES.json").read_text())[0]
    path=EXP/f"replays/episode-{source['episode']}-replay.json"
    replay=json.loads(path.read_text())
    offsets=[];values=[]
    for step in replay["steps"][1:]:
        action=step[source["seat"]]["action"]
        offsets.append(len(values))
        units=[action["farmer"],*action["hands"]]
        values.extend([len(units),len(action["market"])])
        for unit in units:values.extend(triple(unit))
        for order in action["market"]:values.extend(triple(order,True))
    assert len(offsets)==719
    (TARGET/"source").mkdir(parents=True,exist_ok=True)
    code="// Exact recorded action trace, not the source player's general policy.\n"
    code+="inline constexpr int offsets[719]={"+",".join(map(str,offsets))+"};\n"
    code+="inline constexpr int values[]={\n"+",\n".join(",".join(map(str,values[i:i+100])) for i in range(0,len(values),100))+"\n};\n"
    (TARGET/"source/tape.inc").write_text(code)
    (TARGET/"IMPORT.json").write_text(json.dumps({**source,"replay_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
        "changes":["Exact recorded action trace translated to C++ numeric data", "Active worker count normalized to local API"],
        "scope":"One observed fixed course, with no inferred branching or market repair. Not the original player's complete agent.",
        "reuse":"Public Kaggle replay; user explicitly authorized schedule reuse. No separate source code license supplied."},indent=2)+"\n")
    print(source,"actions",len(offsets),"integers",len(values))


if __name__=="__main__":main()
