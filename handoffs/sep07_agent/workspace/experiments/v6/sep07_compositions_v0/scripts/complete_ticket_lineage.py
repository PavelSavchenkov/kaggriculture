"""Attach exact external provenance and local compiler snapshots to a C++ run."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    assert run.is_relative_to(EXP)
    sources = json.loads((EXP / "league/top_replay_library/IMPORT.json").read_text())["programs"]
    paths = [EXP / "src/search_animal_tickets.cpp"]
    paths += [EXP / "include" / name for name in ["animal_ticket.hpp", "terminal_layer.hpp", "ticket_trace.hpp", "ticket_estimate.hpp", "market_tape.hpp", "biology.hpp", "evaluation.hpp", "profile.hpp"]]
    snapshot = run / "compiler_snapshot"
    snapshot.mkdir(exist_ok=True)
    hashes = {}
    for path in paths:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        target = snapshot / path.name
        if target.exists():
            assert hashlib.sha256(target.read_bytes()).hexdigest() == digest, target
        else:
            shutil.copy2(path, target)
        hashes[str(path.relative_to(EXP))] = digest
    for manifest in sorted((run / "proposals").glob("*/IMPORT.json")):
        data = json.loads(manifest.read_text())
        data["source"] = sources[data["source_program"]]
        data["local_compiler_sha256"] = hashes
        data["snapshot_scope"] = "Compiler source at lineage attachment; verify binary build hash for exact executed version"
        data["reuse"] = "User authorized borrowing public replay schedules; no separate replay code license supplied"
        parent = manifest.parent / "PARENT.json"
        if parent.exists():
            data["parent"] = json.loads(parent.read_text())
            data["terminal_component_lineage"] = json.loads((EXP / "candidates/justin_recall_v0/IMPORT.json").read_text())["terminal_ideas"]
        manifest.write_text(json.dumps(data, indent=2) + "\n")
    (snapshot / "HASHES.json").write_text(json.dumps(hashes, indent=2) + "\n")


if __name__ == "__main__":
    main()
