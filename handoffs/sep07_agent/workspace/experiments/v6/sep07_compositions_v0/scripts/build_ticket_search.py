"""Build a content-addressed C++ composition search; Python only orchestrates."""
import hashlib
import json
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    sources = [EXP / "src/search_animal_tickets.cpp", EXP / "league/public_router/source/agent.cpp", EXP / "league/top_replay_library/source/agent.cpp", EXP / "league/king_rc4/source/agent.cpp"]
    flags = ["-std=c++20", "-O3", "-march=native", "-mtune=native", "-DNDEBUG", "-fno-exceptions", "-fno-rtti", "-fno-math-errno", "-fno-semantic-interposition", "-fno-plt", "-flto", "-pthread"]
    compiler = subprocess.check_output(["conda", "run", "-n", "kaggriculture", "g++", "--version"], text=True)
    digest = hashlib.sha256(json.dumps([flags, compiler]).encode())
    roots = [EXP / "include", EXP / "src", EXP / "candidates", EXP / "runs", EXP / "league/public_router", EXP / "league/top_replay_library", EXP / "league/king_rc4", ROOT / "fast_game_engine", ROOT / "agents/common/api", ROOT / "agents/common/runtime"]
    for root in roots:
        for path in sorted(root.rglob("*")):
            if root == EXP / "runs" and path.suffix == ".json" and path.name != "agent.json":
                continue
            if path.is_file() and path.suffix in {".hpp", ".cpp", ".inc", ".json", ".h"}:
                digest.update(str(path.relative_to(ROOT)).encode())
                digest.update(path.read_bytes())
    digest.update(Path(__file__).read_bytes())
    directory = EXP / "build" / ("tickets_" + digest.hexdigest()[:20])
    binary = directory / "search"
    if not binary.exists():
        directory.mkdir(parents=True, exist_ok=True)
        command = ["conda", "run", "-n", "kaggriculture", "g++", *flags, "-I", str(ROOT), *map(str, sources), "-o", str(binary)]
        (directory / "build.json").write_text(json.dumps({"command": command, "compiler": compiler}, indent=2) + "\n")
        subprocess.run(command, check=True, cwd=ROOT)
    print(binary)


if __name__ == "__main__":
    main()
