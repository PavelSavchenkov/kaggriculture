"""Freeze experiment code and hashes of permitted persistent dependencies."""
import argparse
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "snapshots" / args.name
    output.mkdir(parents=True, exist_ok=False)
    files = [EXP / "CMakeLists.txt"] + [path for folder in ["include", "source", "scripts", "docs"]
                                      for path in (EXP / folder).rglob("*") if path.is_file() and "__pycache__" not in path.parts]
    entries = []
    for path in sorted(files):
        relative = path.relative_to(EXP)
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, target)
        entries.append({"path": str(relative), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    dependencies = []
    for folder in ["day_solver/src", "day_solver/include", "fast_game_engine"]:
        for path in sorted((REPO / folder).rglob("*")):
            if path.is_file() and path.suffix in [".hpp", ".cpp", ".h", ".py"] and "__pycache__" not in path.parts:
                dependencies.append({"path": str(path.relative_to(REPO)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    manifest = {"utc": datetime.now(timezone.utc).isoformat(), "files": entries, "persistent_dependencies": dependencies}
    (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Frozen{len(entries)} experiment files; hashed{len(dependencies)} persistent source dependencies")


if __name__ == "__main__":
    main()
