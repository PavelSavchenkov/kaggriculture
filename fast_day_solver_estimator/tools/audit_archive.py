"""Record the checkpoint checksum and check its extraction paths."""
import hashlib
import argparse
import json
import posixpath
import subprocess
import tarfile
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]


def relative_path(name):
    normalized = posixpath.normpath(name)
    assert not normalized.startswith("/") and normalized != ".." and not normalized.startswith("../"), name
    return normalized


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    archive = args.archive.resolve()
    digest = hashlib.sha256()
    with archive.open("rb") as stream:
        while block := stream.read(8 * 1024 * 1024):
            digest.update(block)
    names = set()
    byte_count = files = links = 0
    process = subprocess.Popen(["zstd", "-dc", str(archive)], stdout=subprocess.PIPE)
    with tarfile.open(fileobj=process.stdout, mode="r|") as contents:
        for member in contents:
            name = relative_path(member.name)
            assert name not in names, name
            names.add(name)
            assert member.isfile() or member.isdir() or member.issym() or member.islnk(), name
            if member.issym():
                relative_path(posixpath.join(posixpath.dirname(name), member.linkname))
                links += 1
            elif member.islnk():
                relative_path(member.linkname)
                links += 1
            elif member.isfile():
                files += 1
                byte_count += member.size
        while process.stdout.read(8 * 1024 * 1024):
            pass
    assert process.wait() == 0, "zstd decompression/checksum failed"
    required = ["README.md", "FINAL_CHECKPOINT.json", "STATUS.json", "RENAMED.json", "CMakeLists.txt"]
    assert all(name in names for name in required)
    assert all(any(name.startswith(prefix + "/") for name in names)
               for prefix in ["runs", "scripts", "models", "data", "docs", "include", "source", "baselines"])
    result = {"archive": "research/checkpoint.tar.zst", "sha256": digest.hexdigest(),
              "bytes": archive.stat().st_size, "uncompressed_file_bytes": byte_count,
              "members": len(names), "regular_files": files, "links": links,
              "path_audit": "passed", "zstd_stream_check": "passed",
              "source_experiment": "experiments/v6/fast_day_solver_estimator",
              "excluded": ["./build", "*/__pycache__"],
              "historical_absolute_paths": "Preserved inside evidence; not extraction destinations."}
    assert not args.output.exists(), args.output
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
