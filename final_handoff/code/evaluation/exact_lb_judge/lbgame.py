"""One exact Local-LB game with the LB's own code (snapshot src/lb: load_config, load_agent, run_single_game; official CPU engine,
which the LB verified identical to its native engine). The challenger is an agent folder (main.py + bridge + model/); opponents come
from the snapshot's agents/. Writes a JSON record (money, result, rewards, statuses, error, elapsed).
usage: lbgame.py <snapshot> <challenger agent dir> <opponent id> <seed> <challenger seat 0|1> <out.json>"""
import json, sys, time
from dataclasses import replace
from pathlib import Path
snap, chal_dir, opp, seed, seat, out = Path(sys.argv[1]), Path(sys.argv[2]).resolve(), sys.argv[3], int(sys.argv[4]), int(sys.argv[5]), Path(sys.argv[6])
sys.path.insert(0, str(snap / "src"))
from lb.config import load_config
from lb.match import load_agent, run_single_game
config = load_config(snap / "config/leaderboard.yaml")
config = replace(config, match=replace(config.match, cpu_backend="official"))
chal = load_agent(chal_dir.parent, chal_dir.name, config)
oppf = load_agent(snap / "agents", opp, config)
a0, a1 = (chal, oppf) if seat == 0 else (oppf, chal)
t = time.time()
o = run_single_game(a0, a1, config=config, seed=seed, challenger_seat=seat)
rec = {"challenger": chal_dir.name, "opponent": opp, "seed": seed, "seat": seat, "result": o.get("result"), "money": o.get("money"),
       "rewards": o.get("rewards"), "statuses": o.get("statuses"), "forfeit": o.get("forfeit"), "error": o.get("error"),
       "agent_failures": o.get("agent_failures", []), "wall": time.time() - t}
out.write_text(json.dumps(rec))
