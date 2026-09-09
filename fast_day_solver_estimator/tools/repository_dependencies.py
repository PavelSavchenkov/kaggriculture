"""Verify or copy the exact shared dependencies recorded with this package."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]
REPO = PACKAGE.parent


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(8 * 1024 * 1024):
            result.update(block)
    return result.hexdigest()


def verify(repository=REPO):
    manifest = json.loads((PACKAGE / "REPOSITORY_DEPENDENCIES.json").read_text())
    count = 0
    for group in manifest["groups"].values():
        directory = repository / group["repository_directory"]
        for name, expected in group["files"].items():
            path = directory / name
            assert path.is_file(), f"missing shared dependency: {path}"
            assert digest(path) == expected, f"shared dependency changed: {path}"
            count += 1
    return manifest, count


def copy_dependencies(destination, repository=REPO):
    manifest, count = verify(repository)
    for group in manifest["groups"].values():
        relative = Path(group["repository_directory"])
        for name in group["files"]:
            source = repository / relative / name
            target = destination / relative / name
            assert not target.exists(), target
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=REPO)
    args = parser.parse_args()
    _, count = verify(args.repo.resolve())
    print(json.dumps({"status": "passed", "shared_dependency_files": count,
                      "scope": "Repository file contents only; Git tracking status is not inspected."}))


if __name__ == "__main__":
    main()
