"""Opening / conditioning sweep on the Kaggle proxy (82 pinned v28 Kaggle games, build_m/replay_games_dc11).
Each spec line: name key=value ...; the arm's model folder models/sw_<name> starts as a copy of the base (default
models/dc11_v29_ens5: v29 + ens5, timing=1) and applies:
  main=<dir>        main network from <dir> (model.bin, .condition, .features)
  opening=D,S       .opening "D S" (DSM style 7 for D days is the default); opening=0 removes the pin
  condition=a,b,c   .condition (enabled, strength/200, day index/40; default 1,0.75,1.025)
  style=S           .style: teacher style after the opening (default none = all-zero)
  nolm=1            decode without land_match
  reach_days=a,b    decode: herd-reach days (default 6,14)
  dc11=k=v,...      dc11 options sidecar (default timing=1)
  members=a,b,...   ensemble members from these dirs ("-": none)
  forecast=<dir>    copy <dir>/model.bin.forecast_tf (+ .forecast_tf_exact / .forecast_tf_intra if present)
Arms run P at a time (1 thread each); an existing non-empty CSV is kept.
usage: sweep_kaggle.py <spec file> [P]   -> reports/kaggle_pinned/sw_<name>.csv"""
import os, shutil, subprocess, sys, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIST = "/home/pavel/Programming/kaggriculture/work/sep25_bc_weakness/frozen/kdc81b_list.txt"


def build(name, opts):
    base = ROOT / opts.pop("base", "models/dc11_v29_ens5")
    d = ROOT / "models" / f"sw_{name}"
    if d.exists():
        shutil.rmtree(d)
    shutil.copytree(base, d)
    m = d / "model.bin"
    if "main" in opts:
        src = ROOT / opts.pop("main")
        for suffix in ["", ".condition", ".features"]:
            shutil.copy(src / f"model.bin{suffix}", Path(f"{m}{suffix}"))
    if "opening" in opts:
        value = opts.pop("opening")
        Path(f"{m}.opening").unlink(missing_ok=True)
        if value != "0":
            Path(f"{m}.opening").write_text(value.replace(",", " ") + "\n")
    if "condition" in opts:
        Path(f"{m}.condition").write_text(opts.pop("condition").replace(",", " ") + "\n")
    if "style" in opts:
        Path(f"{m}.style").write_text(opts.pop("style") + "\n")
    if "reach_days" in opts:  # decode: herd-reach days, e.g. reach_days=3,14
        lines = [line for line in Path(f"{m}.decode").read_text().splitlines() if not line.startswith("reach_days")]
        Path(f"{m}.decode").write_text("".join(line + "\n" for line in lines) + "reach_days " + opts.pop("reach_days").replace(",", " ") + "\n")
    if opts.pop("nolm", None) == "1":
        lines = Path(f"{m}.decode").read_text().splitlines()
        Path(f"{m}.decode").write_text("".join(line + "\n" for line in lines if not line.startswith("land_match")))
    if "dc11" in opts:  # dc11 options sidecar, e.g. dc11=timing=1,landtrim=4
        Path(f"{m}.dc11").write_text(opts.pop("dc11").replace(",", " ") + "\n")
    if "members" in opts:  # replaces the ensemble members (model.bin of each dir; "-" = no ensemble)
        value = opts.pop("members")
        shutil.rmtree(d / "members", ignore_errors=True)
        Path(f"{m}.ensemble").unlink(missing_ok=True)
        if value != "-":
            (d / "members").mkdir()
            names = []
            for src in value.split(","):
                name = Path(src).name
                for suffix in ["", ".condition", ".features"]:
                    shutil.copy(ROOT / src / f"model.bin{suffix}", d / "members" / f"{name}.bin{suffix}")
                names.append(f"members/{name}.bin")
            Path(f"{m}.ensemble").write_text("\n".join(names) + "\n")
    if "forecast" in opts:
        src = ROOT / opts.pop("forecast")
        for suffix in [".forecast_tf", ".forecast_tf_exact", ".forecast_tf_intra"]:
            if (src / f"model.bin{suffix}").exists():
                shutil.copy(src / f"model.bin{suffix}", Path(f"{m}{suffix}"))
    assert not opts, f"unknown keys {opts}"
    return m


def main():
    spec, slots = sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 3
    lock = Path(spec + ".lock")  # one runner per spec: a second one would rebuild the folders of arms in flight
    if lock.exists() and Path(f"/proc/{lock.read_text().strip()}").exists():
        sys.exit(f"{spec}: runner {lock.read_text().strip()} is active")
    lock.write_text(str(os.getpid()))
    arms = []
    for line in open(spec):
        words = line.split("#")[0].split()
        if words:
            arms.append((words[0], dict(w.split("=", 1) for w in words[1:])))
    env = {k: v for k, v in os.environ.items() if k not in ("SHOP_CRN", "DC11_OPTIONS")}
    env.update(REPLAY_SHOPS="1", REPLAY_WEEDS="1")
    running = []
    for name, opts in arms:
        out = ROOT / "reports/kaggle_pinned" / f"sw_{name}.csv"
        if out.exists() and out.stat().st_size > 0:
            continue
        while len(running) >= slots:
            running = [p for p in running if p.poll() is None]
            time.sleep(10)
        model = build(name, dict(opts))
        print(f"{time.strftime('%H:%M')} start {name} {opts}", flush=True)
        running.append(subprocess.Popen(["nice", "-n", "5", str(ROOT / "build_m/replay_games_dc11"), LIST, "1", str(out)],
                                        env={**env, "BC_OPUS_MODEL": str(model)}, stdout=subprocess.DEVNULL,
                                        stderr=subprocess.DEVNULL))
    for p in running:
        p.wait()
    print(f"{time.strftime('%H:%M')} done", flush=True)


if __name__ == "__main__":
    main()
