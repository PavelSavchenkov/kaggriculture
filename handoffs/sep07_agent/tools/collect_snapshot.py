"""Historical snapshot collector; normal reproduction uses reproduce.py."""
import gzip
import hashlib
import json
import shutil
import subprocess
import time
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HANDOFF = Path(__file__).resolve().parents[1]
ROOT = HANDOFF.parents[1]
EXP_REL = Path("experiments/v6/sep07_compositions_v0")
CODE = {".cpp", ".hpp", ".h", ".inc", ".py", ".md", ".sh", ".cmake", ".ipynb"}
SKIP = {".o", ".a", ".so", ".pyc", ".pyo"}


def collect(job):
    source, relative = job
    essential = str(relative) == "submitted/teammate_reference/agent.so"
    if (source.suffix in SKIP and not essential) or "__pycache__" in source.parts:
        return {"path": str(relative), "omitted": "rebuildable object/library or Python cache"}
    with source.open("rb") as stream:
        magic = stream.read(4)
    if magic == b"\x7fELF" and not essential:
        return {"path": str(relative), "omitted": "rebuildable executable"}
    data = source.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    record = {"path": str(relative), "sha256": digest, "bytes": len(data)}
    submitted = relative.parts[0] == "submitted"
    archived_source = "source_snapshot" in relative.parts or "build" in relative.parts
    readable = (essential or (source.suffix in CODE and not archived_source and source.suffix != ".ipynb") or
                len(data) < 131072 or
                (submitted and source.suffix in {".json", ".gz"} and len(data) < 10000000))
    if readable:
        target = HANDOFF / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        record["storage"] = "file"
    else:
        blob = Path("evidence/blobs") / digest[:2] / (digest + ".gz")
        target = HANDOFF / blob
        target.parent.mkdir(parents=True, exist_ok=True)
        compressed = gzip.compress(data, compresslevel=6, mtime=0)
        # Same hashes have the same deterministic bytes, including concurrent
        # copies of an identical historical dependency.
        target.write_bytes(compressed)
        record["storage"] = str(blob)
        record["stored_bytes"] = len(compressed)
    return record


def main():
    inventory = HANDOFF / "evidence/inventory.json"
    assert not inventory.exists()
    jobs = []
    roots = [(ROOT / EXP_REL, Path("workspace") / EXP_REL),
             (ROOT / "submissions/sep7-shop-herd-adaptive-v1", Path("submitted")),
             (ROOT / "fast_game_engine", Path("workspace/fast_game_engine")),
             (ROOT / "agents/common/api", Path("workspace/agents/common/api")),
             (ROOT / "agents/common/runtime", Path("workspace/agents/common/runtime")),
             (ROOT / "prompts", Path("workspace/prompts"))]
    for name in json.loads((ROOT / EXP_REL / "configs/league.json").read_text()).values():
        if name.startswith("agents/"):
            roots.append((ROOT / name, Path("workspace") / name))
    for source, destination in roots:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                jobs.append((path, destination / path.relative_to(source)))
    records = []
    started = time.monotonic()
    with ThreadPoolExecutor(max_workers=4) as workers:
        for index, record in enumerate(workers.map(collect, jobs)):
            records.append(record)
            if index % 1000 == 0:
                print(index, "/", len(jobs), "files", round(time.monotonic() - started, 1), "seconds", flush=True)
    inventory.parent.mkdir(parents=True, exist_ok=True)
    inventory.write_text(json.dumps({"description": "Complete session and exact submitted text/data snapshot. Binaries and caches are rebuildable and explicitly listed as omitted. Large evidence and historical build inputs are content-addressed gzip blobs; hydrate restores their original paths and bytes.",
                                     "files": records}, indent=2) + "\n")
    tracked = subprocess.check_output(["git", "ls-files", "-z", "day_solver"], cwd=ROOT).split(b"\0")
    dependencies = {}
    for name in tracked:
        if name:
            path = ROOT / name.decode()
            dependencies[name.decode()] = hashlib.sha256(path.read_bytes()).hexdigest()
    dependency = {"repository_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                  "description": "Existing tracked V30 scheduler, including bundled Linux x86-64 libraries. The reproduction workspace links to this repository dependency; no live experiment or untracked source is required.",
                  "files": dependencies}
    (HANDOFF / "evidence/day_solver_dependency.json").write_text(json.dumps(dependency, indent=2) + "\n")
    link = HANDOFF / "workspace/day_solver"
    if not link.exists():
        link.symlink_to("../../../day_solver", target_is_directory=True)
    goal = Path("/home/pavel/.codex/attachments/79a4f769-6620-487e-82bd-39bbedd94ffa/goal-objective.md")
    shutil.copy2(goal, HANDOFF / "docs/original_goal.md")
    print("snapshot complete", len(records), Counter(r.get("storage", "omitted") == "file" for r in records), flush=True)


if __name__ == "__main__":
    main()
