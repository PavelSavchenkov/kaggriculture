"""Kaggle kernel check of the exact submission archive: runtime info, a from-source build of the
bridge, and full official-environment games with the shipped and the rebuilt library. The kernel
image ships kaggle-environments 1.29.3 (older rules: 10x hand cost, $600 cows, other price curves),
so the competition's 1.32.7 is installed offline from the dataset first."""

import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path


ARCHIVE_SHA256 = "f8c82bd813f7ab3e63adef8383a7990fd902f40f292045672d14e5105cf5ac17"
ENVIRONMENT = "1.32.7"
INPUT = next(Path("/kaggle/input").rglob("submission.tar.gz.bin")).parent
WORK = Path("/kaggle/working")


def run(*command, cwd=WORK):
    print("RUN", " ".join(map(str, command)), flush=True)
    result = subprocess.run([str(part) for part in command], cwd=cwd, capture_output=True, text=True)
    print(result.stdout[-20000:], result.stderr[-20000:], sep="\n", flush=True)
    result.check_returncode()
    return result.stdout


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


cpu = next(line for line in Path("/proc/cpuinfo").read_text().splitlines() if line.startswith("model name"))
report = {
    "platform": platform.platform(), "libc": platform.libc_ver(), "python": sys.version,
    "cpu": cpu.split(":", 1)[1].strip(), "cpu_count": os.cpu_count(),
    "gcc": run("g++", "--version").splitlines()[0], "cmake": run("cmake", "--version").splitlines()[0],
}
print("RUNTIME", json.dumps(report), flush=True)

wheel = WORK / f"kaggle_environments-{ENVIRONMENT}-py3-none-any.whl"
shutil.copyfile(INPUT / f"{wheel.name}.bin", wheel)
run(sys.executable, "-m", "pip", "install", "--no-deps", "--no-index", "--force-reinstall", wheel)
wheel.unlink()

archive = WORK / "submission.tar.gz"
shutil.copyfile(INPUT / "submission.tar.gz.bin", archive)
report["archive_sha256"] = sha256(archive)
assert report["archive_sha256"] == ARCHIVE_SHA256, report["archive_sha256"]

source = WORK / "source_tree"
with tarfile.open(INPUT / "source.tar.gz.bin", "r:gz") as sources:
    sources.extractall(source, filter="data")
build = WORK / "build"
run("cmake", "-S", source / "standalone", "-B", build)
run("cmake", "--build", build, "-j", str(os.cpu_count()))
rebuilt = build / "libopus_lb_bridge.so"
report["rebuilt_library_sha256"] = sha256(rebuilt)
run("ldd", rebuilt)

verify = source / "verify_submission.py"
run(sys.executable, verify, "--archive", archive, "--output", WORK / "verify_shipped.json")
run(sys.executable, verify, "--archive", archive, "--library", rebuilt, "--quick",
    "--output", WORK / "verify_rebuilt.json")
report["shipped"] = json.loads((WORK / "verify_shipped.json").read_text())
report["rebuilt"] = json.loads((WORK / "verify_rebuilt.json").read_text())
assert report["shipped"]["kaggle_environments"] == report["rebuilt"]["kaggle_environments"] == ENVIRONMENT
report["rebuilt_same_rewards"] = (report["shipped"]["games"]["self_seed1"]["rewards"]
                                  == report["rebuilt"]["games"]["self_seed1"]["rewards"])
shutil.rmtree(build)
shutil.rmtree(source)
archive.unlink()
(WORK / "check_results.json").write_text(json.dumps(report, indent=2) + "\n")
print("CHECK_RESULTS", json.dumps(report), flush=True)
