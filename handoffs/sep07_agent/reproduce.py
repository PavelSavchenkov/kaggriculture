"""Portable reproduction entry point. Run through conda's kaggriculture env."""
import argparse
import gzip
import hashlib
import json
import os
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
WORK = HERE / "_work"
EXP = Path("experiments/v6/sep07_compositions_v0")
ENV = ["conda", "run", "--no-capture-output", "-n", "kaggriculture"]
FLAGS = ["-std=c++20", "-O3", "-march=native", "-mtune=native", "-DNDEBUG", "-fno-exceptions",
         "-fno-rtti", "-fno-math-errno", "-fno-semantic-interposition", "-fno-plt", "-pthread", "-flto"]


def read(path):
    return json.loads(path.read_text())


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record_bytes(record):
    if record["storage"] == "repository":
        data = (REPO / record["repository_path"]).read_bytes()
    elif record["storage"] == "file":
        data = (HERE / record["path"]).read_bytes()
    else:
        data = gzip.decompress((HERE / record["storage"]).read_bytes())
    assert len(data) == record["bytes"] and hashlib.sha256(data).hexdigest() == record["sha256"], record["path"]
    return data


def check_dependencies(components=None):
    dependency = read(HERE / "evidence/repository_dependencies.json")
    for path, expected in dependency["files"].items():
        if components is not None and Path(path).parts[0] not in components:
            continue
        assert digest(REPO / path) == expected, "Committed dependency differs; see evidence/repository_dependencies.json: " + path
    return dependency


def link_dependencies(destination, prefix):
    dependency = check_dependencies()
    for path, repository_path in dependency["links"].items():
        if not path.startswith(prefix):
            continue
        target = destination / Path(path).relative_to(prefix)
        expected = REPO / repository_path
        if target.is_symlink():
            target.unlink()
        elif target.exists():
            # This removes only the previously generated dependency copy in
            # ignored _work/, never the repository's authoritative resources.
            assert target.is_relative_to(WORK)
            shutil.rmtree(target)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.symlink_to(expected, target_is_directory=True)


def execute(command, cwd=HERE, capture=False):
    WORK.mkdir(exist_ok=True)
    with (WORK / "commands.jsonl").open("a") as log:
        log.write(json.dumps({"cwd": str(cwd), "command": [str(s) for s in command]}) + "\n")
    result = subprocess.run([str(s) for s in command], cwd=cwd, check=True, text=True,
                            stdout=subprocess.PIPE if capture else None)
    return result.stdout


def verify():
    check_dependencies()
    records = read(HERE / "evidence/inventory.json")["files"]
    def check(record):
        if "omitted" in record:
            return
        record_bytes(record)
    with ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(check, records))
    frozen = read(HERE / "submitted/FROZEN.json")
    for path, expected in frozen["files"].items():
        assert digest(HERE / "submitted/source_tree" / path) == expected, path
    for name, expected in read(HERE / "submitted/ARTIFACTS.json").items():
        assert digest(HERE / "submitted" / name) == expected["sha256"], name
    manifest = HERE / "MANIFEST.json"
    if manifest.exists():
        for path, expected in read(manifest)["files_sha256"].items():
            assert digest(HERE / path) == expected, path
        for path, expected in read(manifest)["symlinks"].items():
            assert (HERE / path).is_symlink() and os.readlink(HERE / path) == expected, path
    print("Verified", sum("omitted" not in r for r in records), "snapshot files and", len(frozen["files"]), "frozen submission sources.")


def hydrate(prefix, destination):
    destination = destination.resolve()
    destination.mkdir(parents=True, exist_ok=True)
    count = 0
    for record in read(HERE / "evidence/inventory.json")["files"]:
        if "omitted" in record or not record["path"].startswith(prefix):
            continue
        target = destination / record["path"]
        data = record_bytes(record)
        assert hashlib.sha256(data).hexdigest() == record["sha256"]
        if target.exists():
            assert target.read_bytes() == data, target
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        if record["storage"] == "repository":
            target.symlink_to(REPO / record["repository_path"])
        else:
            target.write_bytes(data)
        count += 1
    print("Restored", count, "files under", destination)


def submitted_workspace():
    check_dependencies({"fast_game_engine"})
    out = WORK / "submitted"
    out.mkdir(parents=True, exist_ok=True)
    for path in (HERE / "submitted").iterdir():
        if path.is_file():
            target = out / path.name
            if not target.exists():
                shutil.copy2(path, target)
    for name in ("source_tree", "teammate_reference"):
        target = out / name
        if not target.exists():
            target.symlink_to(HERE / "submitted" / name, target_is_directory=True)
    return out


def build_submitted(debug=False):
    out = submitted_workspace()
    tree = out / "source_tree"
    top = tree / EXP / "league/top_replay_library/source/agent.cpp"
    teammate = tree / EXP / "league/teammate_shoprouter/source/agent.cpp"
    targets = [("arena", [out / "arena.cpp", top, teammate]),
               ("export_policy", [out / "export_policy.cpp"]),
               ("verify_cpp", [out / "verify_cpp.cpp", top])]
    if debug:
        flags = [f for f in FLAGS if f not in {"-O3", "-DNDEBUG", "-flto"}] + ["-O1", "-g", "-DKAG_VERIFY_MASKS"]
        execute(ENV + ["g++", *flags, "-I", tree, *targets[0][1], "-o", out / "arena_debug"])
        return out
    for name, sources in targets:
        execute(ENV + ["g++", *FLAGS, "-I", tree, *sources, "-o", out / name])
    execute(ENV + ["python", out / "build_submission.py"])
    for name, expected in read(HERE / "submitted/ARTIFACTS.json").items():
        assert digest(out / name) == expected["sha256"], name
    print("Rebuilt main.py, policy_data.json and submission.tar.gz with identical SHA256 hashes.")
    return out


def evaluate_submitted(args):
    out = submitted_workspace()
    binary = out / ("arena_debug" if args.debug else "arena")
    assert binary.exists(), "Run build-submitted first (with --debug when needed)."
    output = out / f'evaluation_{"native" if args.native else "independent"}_{args.seed}_{args.games}_{"debug" if args.debug else "release"}.json'
    command = ENV + [binary, "--a", "shop_herd_s6_m3_g1", "--b", "teammate_shoprouter", "--games", str(args.games),
                     "--seed-start", str(args.seed), "--seat-mode", "both", "--threads", str(args.threads), "--validate", "--output", output]
    if args.native:
        command.append("--native-shops")
    execute(command)
    data = read(output)
    if args.seed == 1150000 and args.games <= 2048:
        reference = read(HERE / "submitted" / ("frozen_cpp_native4096.json" if args.native else "frozen_cpp_4096.json"))
        expected = {(g["seed"], g["seat"]): g for g in reference["games"]}
        ignored = {"unit_faults", "opponent_unit_faults"} if data["validated"] != reference["validated"] else set()
        for game in data["games"]:
            other = expected[(game["seed"], game["seat"])]
            assert {k:v for k,v in game.items() if k not in ignored} == {k:v for k,v in other.items() if k not in ignored}, (game["seed"], game["seat"])
        print("All cash, action hashes, production, trades, discards, and other comparable outcome fields match the frozen evaluation.")
        if ignored:
            print("This run additionally collects unit-fault diagnostics; the original large report left those fields at zero.")
    print(output)


def prepare_session(all_evidence=False):
    session = WORK / "session"
    if not session.exists():
        shutil.copytree(HERE / "workspace", session, symlinks=True)
    link_dependencies(session, "workspace/")
    source_suffixes = {".hpp", ".cpp", ".inc", ".h", ".py", ".cmake", ".sh"}
    restored = 0
    for record in read(HERE / "evidence/inventory.json")["files"]:
        if "omitted" in record or record.get("storage") in {"file", "repository"} or not record["path"].startswith("workspace/"):
            continue
        relative = Path(record["path"]).relative_to("workspace")
        if not all_evidence and relative.suffix not in source_suffixes:
            continue
        target = session / relative
        if not target.exists():
            data = gzip.decompress((HERE / record["storage"]).read_bytes())
            assert hashlib.sha256(data).hexdigest() == record["sha256"]
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            restored += 1
    original = b"/home/pavel/Programming/kaggriculture"
    changes = []
    for record in read(HERE / "evidence/inventory.json")["files"]:
        if "omitted" in record or record.get("storage") == "repository" or not record["path"].startswith("workspace/"):
            continue
        path = session / Path(record["path"]).relative_to("workspace")
        if not path.is_file() or path.suffix not in source_suffixes | {".json", ".txt", ".log"}:
            continue
        data = path.read_bytes()
        if original in data:
            relocated = str(session).encode()
            marker = b"__SEP07_RELOCATED_WORKSPACE__"
            assert marker not in data
            changed = data.replace(relocated, marker).replace(original, relocated).replace(marker, relocated)
            if changed == data:
                continue
            path.write_bytes(changed)
            changes.append({"path": str(path.relative_to(session)), "original_sha256": hashlib.sha256(data).hexdigest(),
                            "relocated_sha256": hashlib.sha256(changed).hexdigest()})
    report = WORK / "relocation.json"
    previous = read(report) if report.exists() else []
    report.write_text(json.dumps(previous + changes, indent=2) + "\n")
    print("Prepared", session, "restored", restored, "archived inputs; rebased", len(changes), "historical files.")
    return session


def build_agent(args):
    session = prepare_session()
    script = session / EXP / "scripts/build_arena.py"
    command = ENV + ["python", script]
    if not args.generic:
        command += ["--pair", args.agent, args.opponent]
    if args.debug:
        command.append("--debug")
    output = execute(command, cwd=session, capture=True)
    print(output, end="")
    binary = Path(output.strip().splitlines()[-1])
    result = WORK / f"{args.agent}_vs_{args.opponent}.json"
    execute(ENV + [binary, "--a", args.agent, "--b", args.opponent, "--games", str(args.games),
                   "--seed-start", str(args.seed), "--seat-mode", "both", "--threads", str(args.threads),
                   "--validate", "--output", result], cwd=session)
    print(result)


def compile_days(args):
    check_dependencies()
    session = prepare_session()
    build = session / EXP / "build/handoff_day_scheduler"
    execute(ENV + ["cmake", "-S", session / EXP / "scheduler", "-B", build])
    execute(ENV + ["cmake", "--build", build, "--target", args.target, "-j", "4"])
    if args.arguments:
        execute(ENV + [build / args.target, *args.arguments], cwd=session)
    print(build / args.target)


def main():
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("verify")
    prepared = commands.add_parser("prepare-session")
    prepared.add_argument("--all-evidence", action="store_true", help="Restore about9.4GB of raw replay, fixture and evaluation evidence as well as source inputs.")
    rebuild = commands.add_parser("build-submitted")
    rebuild.add_argument("--debug", action="store_true")
    evaluate = commands.add_parser("evaluate-submitted")
    evaluate.add_argument("--native", action="store_true")
    evaluate.add_argument("--debug", action="store_true")
    evaluate.add_argument("--games", type=int, default=2048, help="Seed count; both seats doubles match count.")
    evaluate.add_argument("--seed", type=int, default=1150000)
    evaluate.add_argument("--threads", type=int, default=8)
    packed = commands.add_parser("verify-packed")
    packed.add_argument("--games", type=int, default=32)
    restored = commands.add_parser("hydrate")
    restored.add_argument("--prefix", default="")
    restored.add_argument("--destination", type=Path, default=WORK / "expanded")
    agent = commands.add_parser("build-agent")
    agent.add_argument("agent")
    agent.add_argument("--opponent", default="teammate_shoprouter")
    agent.add_argument("--games", type=int, default=32)
    agent.add_argument("--seed", type=int, default=1000)
    agent.add_argument("--threads", type=int, default=8)
    agent.add_argument("--debug", action="store_true")
    agent.add_argument("--generic", action="store_true", help="Compile the entire registered league, then run this pair.")
    days = commands.add_parser("compile-days")
    days.add_argument("--target", default="improve_care")
    days.add_argument("arguments", nargs="*")
    args = parser.parse_args()
    if args.command == "verify":
        verify()
    elif args.command == "prepare-session":
        prepare_session(args.all_evidence)
    elif args.command == "hydrate":
        hydrate(args.prefix, args.destination)
    elif args.command == "build-submitted":
        build_submitted(args.debug)
    elif args.command == "evaluate-submitted":
        evaluate_submitted(args)
    elif args.command == "build-agent":
        build_agent(args)
    elif args.command == "compile-days":
        compile_days(args)
    elif args.command == "verify-packed":
        out = submitted_workspace()
        assert (out / "verify_cpp").exists(), "Run build-submitted first."
        execute(ENV + ["python", out / "verify_submission.py", "--games", str(args.games)])


if __name__ == "__main__":
    main()
