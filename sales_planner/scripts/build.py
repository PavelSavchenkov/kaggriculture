"""Content-addressed C++ executables. Run this script through conda."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parent
FLAGS = ["-std=c++20", "-O3", "-march=native", "-mtune=native", "-DNDEBUG",
         "-fno-exceptions", "-fno-rtti", "-fno-math-errno", "-fno-semantic-interposition", "-fno-plt", "-flto"]


def build(source, extra_sources=(), debug=False):
    flags = ["-std=c++20", "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if debug else FLAGS
    version = subprocess.check_output(["g++", "--version"], text=True)
    files = [Path(source).resolve(), *(Path(p).resolve() for p in extra_sources)]
    roots = [EXP / "include", ROOT / "fast_game_engine", ROOT / "agents/common/api",
             ROOT / "agents/common/runtime"]
    deps = sorted(set(files + [p for root in roots for p in root.rglob("*")
                              if p.is_file() and p.suffix in {".hpp", ".h", ".inc"}]))
    identity = {"compiler": version, "flags": flags, "sources": [str(p) for p in files],
                "sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in deps},
                "cpu": subprocess.check_output(["g++", "-march=native", "-Q", "--help=target"], text=True),
                "builder_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
    digest = hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()[:20]
    directory = EXP / "build" / digest
    binary = directory / files[0].stem
    if not binary.exists():
        directory.mkdir(parents=True, exist_ok=True)
        command = ["g++", *flags, "-I", str(ROOT), "-I", str(EXP / "include"), *map(str, files), "-o", str(binary)]
        subprocess.run(command, check=True)
        (directory / "BUILD.json").write_text(json.dumps({**identity, "command": command}, indent=2) + "\n")
    return binary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    print(build(args.source, debug=args.debug))


if __name__ == "__main__":
    main()
