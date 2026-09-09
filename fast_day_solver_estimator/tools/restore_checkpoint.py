"""Restore the complete research layout without relying on its original machine."""
import argparse
import json
import subprocess
from pathlib import Path

from repository_dependencies import copy_dependencies, digest, verify

PACKAGE = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    parser.add_argument("--archive", type=Path, required=True,
                        help="separately supplied checkpoint.tar.zst matching research/ARCHIVE.json")
    args = parser.parse_args()
    destination = args.destination.resolve()
    assert not destination.exists(), "choose a new empty workspace path"
    manifest = json.loads((PACKAGE / "research/ARCHIVE.json").read_text())
    archive = args.archive.resolve()
    assert digest(archive) == manifest["sha256"], "checkpoint archive checksum differs"
    verify()
    experiment = destination / "experiments/v6/fast_day_solver_estimator"
    experiment.mkdir(parents=True)
    subprocess.run(["tar", "--use-compress-program=zstd", "--no-same-owner", "-xf", str(archive), "-C", str(experiment)], check=True)
    dependency_count = copy_dependencies(destination)
    record = {"archive_sha256": manifest["sha256"], "experiment": str(experiment),
              "historical_paths": "Preserved. Build new runtime manifests in this workspace; do not rewrite frozen evidence.",
              "background_jobs_started": False, "shared_dependency_files_copied": dependency_count}
    (destination / "RESTORED.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(record, indent=2))


if __name__ == "__main__":
    main()
