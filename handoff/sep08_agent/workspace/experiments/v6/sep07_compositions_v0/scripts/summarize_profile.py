"""Compare local exact-game invariants with the independent top-player cohort."""
import argparse
import json
from pathlib import Path
import pandas as pd

EXP = Path(__file__).resolve().parents[1]
ITEMS = "WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP".split()
BONUS_AGES = ({2,3,4},{2,3},{7,8,9,10},{9,11,13,15},set(range(6,13)))


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("result",type=Path)
    parser.add_argument("--baseline",type=Path)
    parser.add_argument("--global-means",type=Path,default=EXP/"research/invariant_means.json")
    args=parser.parse_args()
    data=json.loads(args.result.read_text())
    if args.baseline:
        baseline=json.loads(args.baseline.read_text())
        lookup={(g["seed"],g["seat"]):g for g in baseline["games"]}
        for game in data["games"]:
            old=lookup[(game["seed"],game["seat"])]
            for key in old:
                if key not in {"profile","opponent_profile"}:
                    assert game[key]==old[key], (game["seed"],game["seat"],key)
    rows=[]
    for game in data["games"]:
        profile=game["profile"]
        row={"seed":game["seed"],"seat":game["seat"],"cash":game["cash"],
             "margin":game["cash"]-game["opponent_cash"],"discards":sum(game["discarded"]),
             "hires":profile["hires"],"hire_cost":profile["hire_cost"],"land_buys":profile["land"],
             "land_cost":profile["land_cost"],"weed_digs":profile["weed_digs"],"faults":game["unit_faults"],
             "shed_max":profile["shed_max"],"shed_turns_ge90":profile["shed_ge90"]}
        assert sum(profile["requested"])-sum(profile["successful"])==game["unit_faults"]
        for i,name in enumerate(ITEMS[:9]):
            row[f"produced_{name}"]=game["produced"][i]
            row[f"buy_{name}"]=profile["buys"][i]
            row[f"sell_{name}"]=game["sold"][i]
            row[f"net_{name}"]=game["sold"][i]-profile["buys"][i]
            for side in ("buy","sell"):
                hours=[profile[f"{side}_hours"][h][i] for h in range(24)]
                assert sum(hours)==row[f"{side}_{name}"]
                row[f"{side}_hour_{name}"]=sum(h*n for h,n in enumerate(hours))/sum(hours) if sum(hours) else None
        crops=[l for l in profile["lives"] if l[0]<5]
        cropdays=sum(l[6].bit_count() for l in crops)
        row["crop_days"]=cropdays
        row["crop_water_rate"]=sum(l[7].bit_count() for l in crops)/cropdays if cropdays else 0
        relevant=maximized=0
        for life in crops:
            mask=sum(1<<d for d in range(30) if d-life[5] in BONUS_AGES[life[0]]) & life[6]
            relevant+=mask.bit_count()
            maximized+=(mask & life[7] & life[11]).bit_count()
        row["crop_yield_day_maximized"]=maximized/relevant if relevant else 0
        for item in range(9,12):
            selected=[l for l in profile["lives"] if l[0]==item]
            days=sum(l[6].bit_count() for l in selected)
            row[f"animal_days_{ITEMS[item]}"]=days
            for name,field in (("fed",8),("cared",9),("collected_fertilizer",10)):
                row[f"{name}_{ITEMS[item]}"]=sum(l[field].bit_count() for l in selected)/days if days else None
            row[f"fed_cared_{ITEMS[item]}"]=sum((l[8]&l[9]).bit_count() for l in selected)/days if days else None
            for day in (0,2,5,8,11,17,23,29):
                row[f"{ITEMS[item]}_d{day}"]=sum(bool(l[6]&(1<<day)) for l in selected)
        rows.append(row)
    frame=pd.DataFrame(rows)
    frame.to_csv(args.result.with_suffix(".profile.csv"),index=False)
    means=frame.mean(numeric_only=True).to_dict()
    global_means=json.loads(args.global_means.read_text())["means"]
    comparison={key:{"local":value,"global":global_means[key]} for key,value in means.items() if key in global_means}
    args.result.with_suffix(".profile.json").write_text(json.dumps({"games":len(rows),"means":means,"comparison":comparison},indent=2)+"\n")
    for key in ("cash","discards","hires","hire_cost","land_buys","crop_days","crop_water_rate","crop_yield_day_maximized","faults","weed_digs"):
        print(key,round(means[key],3),"global",global_means.get(key))
    for animal in ITEMS[9:]:
        print(animal,{k:round(v,3) for k,v in means.items() if k.endswith(animal)})


if __name__=="__main__":
    main()
