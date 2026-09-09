"""Publish fixture agents and copy the estimator's portable source dependencies."""
import hashlib
import json
import re
import shutil
from datetime import datetime, timezone
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]
PACKAGE = REPO / "fast_day_solver_estimator"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def publish_agents():
    originals = EXP / "baselines/warm/runs/submission_losses_sep08_001"
    parent = REPO / "agents/external/cow_service_retained_q24_premium_m2"
    records = []
    for name in ["early_structure_cow", "nanare_four_quadrant_course"]:
        source = originals / "proposals" / name
        target = REPO / "agents/external" / name
        assert not target.exists()
        shutil.copytree(source, target)
        shutil.copyfile(target / "README.md", target / "ORIGINAL_README.md")
        paths = [p for p in source.rglob("*") if p.is_file()]
        transforms = []
        if name == "early_structure_cow":
            shutil.copytree(parent / "source", target / "source/deps/parent")
            shutil.copytree(parent / "provenance", target / "provenance/parent")
            shutil.copyfile(parent / "PROVENANCE.json", target / "provenance/PARENT_PROVENANCE.json")
            shutil.copyfile(parent / "SOURCE_MANIFEST.json", target / "provenance/PARENT_SOURCE_MANIFEST.json")
            shutil.copyfile(originals / "source/structure_repair.hpp", target / "source/structure_repair.hpp")
            paths += [p for p in (parent / "source").rglob("*") if p.is_file()]
            paths += [originals / "source/structure_repair.hpp"]
            for path in (target / "source/deps/parent").rglob("*"):
                if not path.is_file(): continue
                content = path.read_text()
                changed = content.replace("catalog_cow_service_retained_q24_premium_m2_", "catalog_early_structure_cow_parent_")
                changed = changed.replace("kag::agents::cow_service_retained_q24_premium_m2", "kag::agents::early_structure_cow_parent")
                changed = re.sub(r'(?m)^(#include\s+")(?:\.\./)+common/(api|runtime)/([^"]+)(")', r'\1agents/common/\2/\3\4', changed)
                changed = re.sub(r'(?m)^(#include\s+")(?:\.\./)+(fast_game_engine/[^"]+)(")', r'\1\2\3', changed)
                if changed != content:
                    path.write_text(changed); transforms.append(str(path.relative_to(target)))
            header = target / "source/agent.hpp"
            content = header.read_text().replace('"agents/external/cow_service_retained_q24_premium_m2/source/agent.hpp"', '"deps/parent/agent.hpp"')
            content = content.replace('"../../../source/structure_repair.hpp"', '"structure_repair.hpp"')
            content = content.replace("kag::agents::cow_service_retained_q24_premium_m2", "kag::agents::early_structure_cow_parent")
            content = content.replace("namespace compositions::early_structure_cow", "namespace kag::agents::early_structure_cow")
            content = content.replace("public early_structure_repair::", "public ::compositions::early_structure_repair::")
            header.write_text(content)
            parent_sources = json.loads((parent / "agent.json").read_text())["sources"]
            cpp_sources = ["source/agent.cpp", *["source/deps/parent/" + str(Path(p).relative_to("source")) for p in parent_sources]]
            lineage = "Locally written day-one structure repair over the externally derived cow_service_retained_q24_premium_m2 policy. Full parent source is included and namespaces are isolated."
        else:
            for filename in ["source/agent.hpp", "source/agent.cpp"]:
                path = target / filename
                path.write_text(path.read_text().replace("namespace compositions::nanare_four_quadrant_course", "namespace kag::agents::nanare_four_quadrant_course"))
            cpp_sources = ["source/agent.cpp"]
            lineage = "Exact recorded course from nanare, episode 106867299 seat 0; source reward 122310. It is a replay fixture, not the original adaptive policy."
        write_json(target / "agent.json", {"format_version": 1, "name": name, "header": "source/agent.hpp",
                   "type": f"kag::agents::{name}::Agent", "sources": cpp_sources})
        provenance = {"exported_utc": datetime.now(timezone.utc).isoformat(), "classification": "external",
                      "lineage": lineage, "rating": "Unknown for this packaged agent.",
                      "optimization_status": "Cataloged as an estimator benchmark fixture at explicit user request; no new strategy promotion or submission.",
                      "reuse": "Retain attribution and inherited source licenses; no new source-code license is inferred from public availability.",
                      "changes": "Include relocation and namespace isolation only; policy actions are intended to remain identical. See package validation for complete-course parity.",
                      "renamed_parent_files": transforms,
                      "source_sha256": {str(p.relative_to(REPO)): sha(p) for p in paths}}
        write_json(target / "PROVENANCE.json", provenance)
        write_json(target / "external.json", {"source": lineage, "rating": "unknown", "parity": "Package full-course parity check required", "reuse": provenance["reuse"]})
        (target / "README.md").write_text(f"# {name}\n\n{lineage}\n\nThis is a benchmark fixture used by `fast_day_solver_estimator/`. The manifest lists every implementation source; only shared agent API/engine headers are external dependencies. Parent code is retained locally where needed. No strategy promotion or submission is implied.\n\nSee `ORIGINAL_README.md`, `PROVENANCE.json`, and package validation for lineage, restrictions, and action parity.\n")
        write_json(target / "SOURCE_MANIFEST.json", {str(p.relative_to(target)): sha(p) for p in target.rglob("*") if p.is_file()})
        records.append({"name": name, "path": str(target.relative_to(REPO)), "type": f"kag::agents::{name}::Agent"})
    return records


def main():
    assert not PACKAGE.exists()
    agents = publish_agents()
    PACKAGE.mkdir()
    for name in ["include", "source", "models", "baselines"]:
        shutil.copytree(EXP / name, PACKAGE / name)
    (PACKAGE / "docs").mkdir()
    shutil.copytree(EXP / "docs", PACKAGE / "docs/research")
    for name in ["IDEAS_LEDGER.md", "PROFILING_LEDGER.md", "PROGRESS.md"]:
        shutil.copyfile(EXP / name, PACKAGE / "docs/research" / name)
    shutil.copytree(EXP / "scripts", PACKAGE / "development/scripts", ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
    solver = PACKAGE / "vendor/day_solver"; solver.mkdir(parents=True)
    for name in ["include", "src", "lib", "runtime", "vendor", "cmake", "licenses", "docs", "schemas", "tests"]:
        shutil.copytree(REPO / "day_solver" / name, solver / name, symlinks=True)
    for name in ["CMakeLists.txt", "with_runtime.sh", "README.md"]:
        shutil.copyfile(REPO / "day_solver" / name, solver / name)
    (solver / "with_runtime.sh").chmod(0o755)
    snapshot = PACKAGE / "vendor/repo/agents"
    for name in ["common/api", "common/runtime", "external/early_structure_cow", "external/nanare_four_quadrant_course", "external/cow_service_retained_q24_premium_m2"]:
        shutil.copytree(REPO / "agents" / name, snapshot / name, symlinks=True)
    for path in list((PACKAGE / "baselines").rglob("*.hpp")) + [PACKAGE / "include/warm_shop_season.hpp"]:
        content = path.read_text()
        for name in ["early_structure_cow", "nanare_four_quadrant_course"]:
            old = f"baselines/warm/runs/submission_losses_sep08_001/proposals/{name}/source/agent.hpp"
            # The original season uses relative paths; replace either spelling.
            content = content.replace(old, f"agents/external/{name}/source/agent.hpp")
            content = content.replace(f"../../proposals/{name}/source/agent.hpp", f"agents/external/{name}/source/agent.hpp")
            content = content.replace(f"compositions::{name}::Agent", f"kag::agents::{name}::Agent")
        path.write_text(content)
    (PACKAGE / "evidence").mkdir()
    write_json(PACKAGE / "evidence/EXPORT.json", {"utc": datetime.now(timezone.utc).isoformat(),
               "experiment_source": str(EXP.relative_to(REPO)), "published_agents": agents,
               "dependency_policy": "Frozen local solver/engine and agent dependency snapshots are included. No Git command or assumption about machine-local uncommitted dependencies was used."})
    print(json.dumps({"package": str(PACKAGE), "agents": agents}, indent=2))


if __name__ == "__main__":
    main()
