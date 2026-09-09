"""Verify the frozen delivery, or explicitly record a new local manifest."""
import argparse
import hashlib
import json
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]
MANIFEST = PACKAGE / "MANIFEST.json"


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(8 * 1024 * 1024):
            result.update(block)
    return result.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true", help="explicitly replace the delivery manifest")
    args = parser.parse_args()
    if args.write:
        files = {}
        for path in sorted(PACKAGE.rglob("*")):
            relative = path.relative_to(PACKAGE)
            if relative.parts[0].startswith("build") or "__pycache__" in relative.parts or path == MANIFEST:
                continue
            if relative.parts[0] == "research" and path.name.endswith(".tar.zst"):
                continue
            if path.is_symlink():
                assert path.exists() and path.resolve().is_relative_to(PACKAGE), path
            if path.is_file():
                files[str(relative)] = {"bytes": path.stat().st_size, "sha256": digest(path)}
        MANIFEST.write_text(json.dumps({"format_version": 1, "algorithm": "sha256", "files": files,
                                       "excluded": ["build*/", "__pycache__/", "research/*.tar.zst", "MANIFEST.json"]}, indent=2) + "\n")
    manifest = json.loads(MANIFEST.read_text())
    for name, record in manifest["files"].items():
        path = (PACKAGE / name).resolve()
        assert path.is_relative_to(PACKAGE) and path.is_file(), name
        assert path.stat().st_size == record["bytes"] and digest(path) == record["sha256"], name
    print(json.dumps({"status": "passed", "files": len(manifest["files"]), "manifest": "MANIFEST.json"}))


if __name__ == "__main__":
    main()
