"""Copy the closed archive's minimal compiler source closure for new diagnostics."""
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]
ARCHIVE = REPO / "experiments/v6/sep07_compositions_v0"
PREFIX = str(ARCHIVE.relative_to(REPO)) + "/"


def main():
    output = EXP / "baselines/warm"
    output.mkdir(exist_ok=False)
    old_cmake = ARCHIVE / "runs/submission_losses_sep08_001/general_quadrant_compile/CMakeLists.txt"
    sources = [Path(p) for p in re.findall(r'"([^"\n]+\.cpp)"', old_cmake.read_text())]
    sources = [p for p in sources if p.name != "compile.cpp"]
    compilers = [ARCHIVE / f"runs/expansion_portfolio_sep08_001/next_compile/{name}"
                 for name in ["compile.cpp", "compile_early_inputs.cpp"]]
    queue = [p for p in sources + compilers if p.is_relative_to(ARCHIVE)]
    copied, persistent = {}, {}
    while queue:
        path = queue.pop().resolve()
        if path in copied:
            continue
        original = path.read_text()
        relative = path.relative_to(ARCHIVE)
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        adapted = original.replace(PREFIX, "baselines/warm/")
        target.write_text(adapted)
        copied[path] = {"source": str(path.relative_to(REPO)), "path": str(target.relative_to(EXP)),
                        "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                        "sha256": hashlib.sha256(target.read_bytes()).hexdigest()}
        for include in re.findall(r'^\s*#include\s+"([^"]+)"', original, re.MULTILINE):
            candidates = [path.parent / include, REPO / include, REPO / "day_solver/include" / include]
            found = next((p.resolve() for p in candidates if p.is_file()), None)
            assert found, (path, include)
            if found.is_relative_to(ARCHIVE):
                queue.append(found)
            else:
                assert found.is_relative_to(REPO), found
                persistent[str(found.relative_to(REPO))] = hashlib.sha256(found.read_bytes()).hexdigest()
        if path.name in ["agent.cpp", "agent.hpp"]:
            folder = path.parent.parent
            for name in ["agent.json", "README.md"]:
                extra = folder / name
                if extra.is_file():
                    queue.append(extra)
    for source in sources:
        if not source.is_relative_to(ARCHIVE):
            persistent[str(source.relative_to(REPO))] = hashlib.sha256(source.read_bytes()).hexdigest()
    lines = ["# Imported compiler dependencies; all experiment files are local copies.", "set(LABOR_WARM_SOURCES"]
    for source in sources:
        path = '${CMAKE_CURRENT_SOURCE_DIR}/baselines/warm/' + str(source.relative_to(ARCHIVE)) if source.is_relative_to(ARCHIVE) else '${REPO}/' + str(source.relative_to(REPO))
        lines.append('  "' + path + '"')
    lines.append(")")
    (output / "sources.cmake").write_text("\n".join(lines) + "\n")
    report = {"utc": datetime.now(timezone.utc).isoformat(), "purpose": "Read-only import for estimator and original warm-compiler diagnostics; no deployment or new official agent.",
        "adaptation": "Only archive-root include prefixes rewritten to experiment-local baselines/warm paths. Relative includes and algorithms preserved.",
        "files": list(copied.values()), "persistent_direct_dependencies": persistent,
        "archive_seal_sha256": hashlib.sha256((ARCHIVE / "SEAL.json").read_bytes()).hexdigest()}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Copied {len(copied)} source/evidence files; {len(persistent)} direct persistent dependencies.")


if __name__ == "__main__":
    main()
