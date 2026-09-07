"""Offline formatting of observed plans/support into immutable C++ data."""
import csv
import json
from collections import defaultdict
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]
TARGET=EXP/"candidates/composition_greedy_v0"


def main():
    data=json.loads((EXP/"research/compositions.json").read_text())
    proposals=sorted(data["proposals"],key=lambda p:(p["rank"],p["episode"],p["seat"]))
    support=defaultdict(lambda:{"hands":[0]*30,"land":[0]*30})
    masks=defaultdict(lambda:[0]*6)
    operations=["FERTILIZE","WATER","FEED","CARE","COLLECT_FERTILIZER","HARVEST"]
    with (EXP/"research/unit_events.csv").open() as stream:
        for row in csv.DictReader(stream):
            if row["operation"] not in operations or not row["origin_day"]:continue
            key=(int(row["episode_id"]),row["team"],int(row["x"]),int(row["y"]),row["source"],int(row["origin_day"]))
            masks[key][operations.index(row["operation"])]|=1<<int(row["day"])
    with (EXP/"research/transactions.csv").open() as stream:
        for row in csv.DictReader(stream):
            entry=support[(int(row["episode_id"]),row["team"])]
            if row["operation"]=="HIRE":entry["hands"][int(row["day"])]+=int(row["actual"])
            if row["operation"]=="BUY_LAND":entry["land"][int(row["day"])]+=int(row["actual"])
    values=[];offsets=[0];hands=[];land=[];metadata=[]
    for p in proposals:
        for instance in sorted(p["instances"],key=lambda i:(i["start_state"],i["y"],i["x"])):
            key=(p["episode"],p["team"],instance["x"],instance["y"],data["items"][instance["item"]],instance["start_day"])
            values.append((instance["item"],max(0,instance["start_state"]-1),instance["end_state"],instance["x"],instance["y"],*masks[key]))
        offsets.append(len(values))
        observed=support[(p["episode"],p["team"])]
        hands.append(observed["hands"])
        quadrants=1;row=[]
        for purchases in observed["land"]:
            quadrants+=purchases;row.append(quadrants)
        land.append(row)
        metadata.append({"program":len(metadata),**{k:p[k] for k in ("id","episode","seat","team","rank","submission","leaderboard_snapshot")}})
    source=TARGET/"source";source.mkdir(parents=True,exist_ok=True)
    code="// Generated replay facts; strategic compilation is C++.\n"
    code+="inline constexpr Intent recorded_intents[] = {\n"+",\n".join("{"+",".join(map(str,v))+"}" for v in values)+"\n};\n"
    code+="inline constexpr int program_offsets[] = {"+",".join(map(str,offsets))+"};\n"
    for name,rows in (("recorded_hands",hands),("recorded_quadrants",land)):
        code+=f"inline constexpr int {name}[72][30] = {{\n"+",\n".join("{"+",".join(map(str,row))+"}"for row in rows)+"\n};\n"
    (source/"programs.inc").write_text(code)
    (TARGET/"PROGRAM_SOURCES.json").write_text(json.dumps(metadata,indent=2)+"\n")
    print("programs",len(proposals),"intents",len(values),"first",metadata[0])


if __name__=="__main__":main()
