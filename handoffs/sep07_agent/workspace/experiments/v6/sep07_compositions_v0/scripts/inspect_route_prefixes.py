"""Offline source compatibility facts; no inferred routing policy or search."""
import ast
import base64
import json
import zlib
from pathlib import Path

from generate_public_routes import SOURCE

EXP=Path(__file__).resolve().parents[1]


def main():
    tree=ast.parse(SOURCE.read_text())
    blob=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
        and any(isinstance(t,ast.Name) and t.id=="_BLOB" for t in n.targets))
    data=json.loads(zlib.decompress(base64.b64decode(blob)))
    public={data["main"]:data["full"]}
    for tail in data["tails"]:public[tail["h"]]=public[tail["parent"]][:tail["at"]]+tail["suffix"]
    routes={"public_"+key:value[:719] for key,value in public.items()}
    metadata=json.loads((EXP/"league/top_replay_library/IMPORT.json").read_text())["programs"]
    for program in (0,4,30,40,54,55,56,57,58,59,72,78,85,89):
        source=metadata[program]
        replay=json.loads((EXP/f"replays/episode-{source['episode']}-replay.json").read_text())
        routes["program_"+str(program)]=[s[source["seat"]]["action"] for s in replay["steps"][1:]]
    report=[]
    for name,a in routes.items():
        if name=="program_55":continue
        b=routes["program_55"]
        first=lambda fields:next((i for i in range(719) if any(a[i][k]!=b[i][k] for k in fields)),719)
        full=first(("farmer","hands","market"));units=first(("farmer","hands"))
        report.append({"other":name,"full_action_prefix":full,"unit_action_prefix":units,
            "other_at_first":a[full] if full<719 else None,"program55_at_first":b[full] if full<719 else None})
    (EXP/"research/route_prefixes_v2.json").write_text(json.dumps(report,indent=2)+"\n")
    for row in report:print(row["other"],"full",row["full_action_prefix"],"units",row["unit_action_prefix"])


if __name__=="__main__":main()
