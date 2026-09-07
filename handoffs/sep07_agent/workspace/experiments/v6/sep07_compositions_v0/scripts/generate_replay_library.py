"""Format all selected public replay actions into a C++ reference library."""
import argparse
import hashlib
import json
from pathlib import Path

from generate_public_routes import triple

EXP=Path(__file__).resolve().parents[1]
TARGET=EXP/"league/top_replay_library"


def main():
    global TARGET
    parser=argparse.ArgumentParser()
    parser.add_argument("--metadata",type=Path,default=EXP/"research/replay_library_sources_v7.json")
    parser.add_argument("--target",type=Path,default=TARGET)
    args=parser.parse_args()
    TARGET=args.target.resolve()
    assert TARGET.is_relative_to(EXP)
    sources=json.loads(args.metadata.read_text())
    rows=[];values=[];metadata=[]
    for source in sources:
        path=EXP/f"replays/episode-{source['episode']}-replay.json"
        raw=path.read_bytes();replay=json.loads(raw);offsets=[]
        for step in replay["steps"][1:]:
            action=step[source["seat"]]["action"]
            offsets.append(len(values));units=[action["farmer"],*action["hands"]]
            values.extend([len(units),len(action["market"])])
            for unit in units:values.extend(triple(unit))
            for order in action["market"]:values.extend(triple(order,True))
        assert len(offsets)==719
        rows.append(offsets);metadata.append({**source,"replay_sha256":hashlib.sha256(raw).hexdigest()})
    (TARGET/"source").mkdir(parents=True,exist_ok=True)
    code="// Exact recorded courses; sources in IMPORT.json. No inferred branches.\n"
    code+=f"inline constexpr int offsets[{len(rows)}][719]={{\n"+",\n".join("{"+",".join(map(str,r))+"}" for r in rows)+"\n};\n"
    code+="inline constexpr int values[]={\n"+",\n".join(",".join(map(str,values[i:i+100])) for i in range(0,len(values),100))+"\n};\n"
    (TARGET/"source/tapes.inc").write_text(code)
    (TARGET/"IMPORT.json").write_text(json.dumps({"programs":metadata,
        "changes":["Exact recorded actions translated to C++ numeric data", "Worker count normalized to current observation",
                   "Engine-ignored PASS/HIRE/BUY_LAND arguments normalized to zero"],
        "reuse":"Public Kaggle replays; user explicitly authorized copying complete schedules. No separate code license supplied.",
        "scope":"Fixed observed courses, not original players complete branching agents"},indent=2)+"\n")
    print("programs",len(rows),"actions",719*len(rows),"integers",len(values))


if __name__=="__main__":main()
