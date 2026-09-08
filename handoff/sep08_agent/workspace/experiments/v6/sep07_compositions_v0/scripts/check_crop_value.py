"""Run complete C++ operational/native checks for the observed-demand crop gate."""
import json
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
NEW = "crop_value_m2_t4"
OLD = "investment_context_guarded_001_best"
GENERIC = EXP / "build/e0f9cbdb226c4e78f49f/arena"
PAIR = EXP / "build/72ccbbff24b4f7a3b177/arena"
DEBUG = EXP / "build/024ea493a599028c8fdb/arena"
OUT = EXP / "runs/crop_value_checks_001"


def main():
    OUT.mkdir()
    records = {}

    def run(name, a, b, games=32, seed=1000, binary=GENERIC, threads=6, extra=()):
        output = OUT / (name + ".json")
        command = ["conda", "run", "-n", "kaggriculture", str(binary), "--a", a, "--b", b,
                   "--games", str(games), "--seed-start", str(seed), "--seat-mode", "both",
                   "--threads", str(threads), "--validate", "--output", str(output), *extra]
        with (OUT / (name + ".log")).open("w") as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        data = json.loads(output.read_text())
        assert len(data["games"]) == 2 * games and all(g["turns"] == 719 for g in data["games"])
        records[name] = {"command": command, "summary": {k: v for k, v in data.items() if k != "games"}}
        (OUT / "RUN.json").write_text(json.dumps(records, indent=2) + "\n")
        return data["games"]

    generic = run("generic64", NEW, "teammate_shoprouter")
    pair = run("pair64", NEW, "teammate_shoprouter", binary=PAIR)
    debug = run("debug64", NEW, "teammate_shoprouter", binary=DEBUG)
    single = run("thread64", NEW, "teammate_shoprouter", threads=1)
    assert generic == pair == debug == single
    run("self16", NEW, NEW, games=8)
    for agent in [NEW, OLD]:
        run(agent + "_pass256", agent, "pass", games=128, seed=1420000)
        run(agent + "_profile64", agent, "public_router", extra=["--profile"])
        for opponent in [OLD, "teammate_shoprouter", "king_rc4", "public_router", "public_router_v5"]:
            run(agent + "_native_" + opponent, agent, opponent, games=128, seed=1420000, extra=["--native-shops"])
        run(agent + "_group_missing", agent, "shop_herd_guarded_001_best", games=512, seed=1360000)
    records["generic_pair_debug_thread_all_game_records_equal"] = True
    (OUT / "CHECKS.json").write_text(json.dumps(records, indent=2) + "\n")
    print("All complete operational checks passed.")


if __name__ == "__main__":
    main()
