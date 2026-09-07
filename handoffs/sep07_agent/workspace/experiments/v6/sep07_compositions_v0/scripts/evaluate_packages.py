"""Orchestrate whole C++ seed batches; all game logic stays in the arena."""
import argparse
import json
import subprocess
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("binary",type=Path)
    parser.add_argument("directory",type=Path)
    parser.add_argument("--agents",nargs="+",required=True)
    parser.add_argument("--opponents",nargs="+",required=True)
    parser.add_argument("--games",type=int,default=128)
    parser.add_argument("--seed-start",type=int,default=1000)
    parser.add_argument("--threads",type=int,default=4)
    args=parser.parse_args()
    catalog=json.loads((EXP/"configs/league.json").read_text())
    unknown=set(args.agents+args.opponents)-set(catalog)-{"pass"}
    if unknown:parser.error(f"Unknown agents: {sorted(unknown)}")
    args.directory.mkdir(parents=True,exist_ok=True)
    rows=[]
    for agent in args.agents:
        for opponent in args.opponents:
            output=args.directory/f"{agent}_vs_{opponent}.json"
            if output.exists():raise FileExistsError(output)
            command=["conda","run","-n","kaggriculture",str(args.binary),"--a",agent,"--b",opponent,
                "--games",str(args.games),"--seed-start",str(args.seed_start),"--threads",str(args.threads),
                "--seat-mode","both","--validate","--output",str(output)]
            subprocess.run(command,check=True)
            result=json.loads(output.read_text())
            rows.append({"agent":agent,"opponent":opponent,"summary":{k:v for k,v in result.items() if k!="games"}})
            (args.directory/"summary.json").write_text(json.dumps(rows,indent=2)+"\n")


if __name__=="__main__":main()
