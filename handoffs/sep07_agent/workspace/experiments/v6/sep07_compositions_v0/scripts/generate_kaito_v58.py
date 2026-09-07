"""Format statically decoded public routes; no strategy search runs in Python."""
import hashlib
import json
from pathlib import Path
from generate_public_routes import triple

EXP=Path(__file__).resolve().parents[1]
TARGET=EXP/"league/kaito_v58"
ROUTES=("_V55_LIVE_ROUTE","_V56_YARN_ROUTE","_V57_PET_ROUTE","_V58_RECOVERY_ROUTE",
    "_V58_SMOOTHIE_ROUTE","_V58_CLONE_ROUTE","_V58_KNOWN_YARN_ROUTE",
    "_V58_ICE_MINIMAX_ROUTE","_V58_BAKERY_YARN_ROUTE","_V58_PIZZA_RECOVERY_ROUTE")


def main():
    directory=EXP/"research/kaito_v58"
    offsets=[];values=[];metadata=[]
    for name in ROUTES:
        path=directory/f"{name}.json"
        raw=path.read_bytes();route=json.loads(raw);row=[]
        assert len(route)==719
        for action in route:
            row.append(len(values))
            units=[action["farmer"],*action["hands"]]
            values.extend((len(units),len(action["market"])))
            for unit in units:values.extend(triple(unit))
            for order in action["market"]:values.extend(triple(order,True))
        offsets.append(row)
        metadata.append({"route":name,"source_sha256":hashlib.sha256(raw).hexdigest()})
    (TARGET/"source").mkdir(parents=True,exist_ok=True)
    code="// Apache-2.0 public notebook route data. Exact provenance: ../IMPORT.json.\n"
    code+="inline constexpr int route_offsets[10][719]={\n"+",\n".join("{"+",".join(map(str,r))+"}" for r in offsets)+"\n};\n"
    code+="inline constexpr int route_values[]={\n"+",\n".join(",".join(map(str,values[i:i+100])) for i in range(0,len(values),100))+"\n};\n"
    (TARGET/"source/routes.inc").write_text(code)
    origin=directory/"upstream.py"
    (TARGET/"IMPORT.json").write_text(json.dumps({"source":"external/kaggriculture/agents/champion-v58/v58_agent.py",
        "url":"https://www.kaggle.com/code/kaitofukami/238-238-known-streams-v58-minimax-closed-loop",
        "authors":["Kaito Fukami","Pilkwang Kim (loader-only fork)"],"license":"Apache-2.0",
        "source_sha256":hashlib.sha256(origin.read_bytes()).hexdigest(),"routes":metadata,
        "changes":["C++ local API and immutable integer tables", "all ten independent controllers update from turn zero, equivalent to replayed warm prefixes",
            "literal active v58 residual configuration; disabled market-maker and defer logic omitted"],
        "rating":"Not established from this notebook headline; fresh full-game evaluation pending",
        "status":"Port under parity verification; do not claim source faithfulness until checks pass"},indent=2)+"\n")
    manifest={"format_version":1,"name":"kaito_v58","header":"source/agent.hpp",
        "type":"compositions::kaito_v58::Agent","sources":["source/agent.cpp"]}
    (TARGET/"agent.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print("actions",719*len(ROUTES),"integers",len(values))


if __name__=="__main__":main()
