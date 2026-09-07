"""Convert exact local profiling records into typed C++ financial fixtures."""
import argparse
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("profile",type=Path)
    parser.add_argument("--output",type=Path,default=EXP/"research/financial_cases.txt")
    parser.add_argument("--two-sided",action="store_true")
    args=parser.parse_args()
    data=json.loads(args.profile.read_text())
    with args.output.open("w") as out:
        out.write(("two_sided_v1 " if args.two_sided else "")+str(len(data["games"]))+"\n")
        for game in data["games"]:
            own=game["profile"];rival=game["opponent_profile"]
            out.write(" ".join(map(str,[game["seed"],game["seat"],game["cash"],len(own["stock_flows"]),len(own["fixed_costs"]),len(rival["flows"])]))+"\n")
            if args.two_sided:out.write(str(sum(row[1] for row in rival["fixed_costs"]))+"\n")
            out.write(" ".join(map(str,game["shops"]))+"\n")
            for records in (own["stock_flows"],own["fixed_costs"],rival["flows"]):
                for record in records:
                    out.write(" ".join(map(str,record))+"\n")
    print("financial_cases",len(data["games"]),args.output)


if __name__=="__main__":
    main()
