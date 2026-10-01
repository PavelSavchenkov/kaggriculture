"""Build the Kaggle agent archive deterministically and record its hashes in BUILD.json."""

import gzip
import hashlib
import io
import json
import tarfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
# main.py, the bridge and every model file (network, sidecars, ensemble members, forecaster, dc11 options).
FILES = ("main.py", "libopus_lb_bridge.so",
         *sorted(str(p.relative_to(ROOT)) for p in (ROOT / "model").rglob("*") if p.is_file()))
DIRECTORIES = sorted({str(Path(name).parent) for name in FILES if "/" in name})


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


compile((ROOT / "main.py").read_text(), "main.py", "exec")
archive_path = ROOT / "submission.tar.gz"
with archive_path.open("wb") as output:
    with gzip.GzipFile(fileobj=output, mode="wb", filename="", mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode="w", format=tarfile.USTAR_FORMAT) as archive:
            for name in DIRECTORIES:
                folder = tarfile.TarInfo(name)
                folder.type, folder.mode, folder.mtime = tarfile.DIRTYPE, 0o755, 0
                archive.addfile(folder)
            for name in FILES:
                data = (ROOT / name).read_bytes()
                info = tarfile.TarInfo(name)
                info.size, info.mode, info.mtime = len(data), 0o755 if name.endswith(".so") else 0o644, 0
                archive.addfile(info, io.BytesIO(data))

report = {name: {"bytes": (ROOT / name).stat().st_size, "sha256": sha256(ROOT / name)}
          for name in (*FILES, "submission.tar.gz")}
(ROOT / "BUILD.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
