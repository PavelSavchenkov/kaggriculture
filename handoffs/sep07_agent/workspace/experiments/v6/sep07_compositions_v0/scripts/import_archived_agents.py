"""Copy selected historical C++ agents and repair only moved include paths."""
import hashlib
import json
import re
import shutil
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
REPLAY = ROOT / "experiments/v2/aug31_four_random_shops_league_foundry/candidates/replay"
ARCHIVE = ROOT / "work/agent_storage_archive_20260901/external"
PACKAGES = {
    "ryo_3000": REPLAY / "replay_ryo_ge3000_95029942",
    "arman_3000": REPLAY / "replay_arman_ge3000_94541153",
    "crop_dusta_3000": REPLAY / "replay_crop_dusta_ge3000_100223989",
    "subramanya_3000": REPLAY / "replay_subramanya_ge3000_96594837",
    "mrkiwi_3000": ARCHIVE / "test_replay_mrkiwi_ge3000_93167917",
    "structured": ARCHIVE / "test_structured_economic_policy",
    "deniz": ARCHIVE / "public_deniz_v111_safe",
}


def main():
    catalog_path = EXP / "configs/league.json"
    catalog = json.loads(catalog_path.read_text())
    for name, origin in PACKAGES.items():
        target = EXP / "league" / name
        if target.exists():
            raise FileExistsError(target)
        target.mkdir()
        hashes = {}
        for path in sorted(origin.rglob("*")):
            if not path.is_file() or path.suffix not in {".cpp", ".hpp", ".inc", ".json", ".md", ".txt"}:
                continue
            relative = path.relative_to(origin)
            if relative.parts[0] in {"build", "tests"}:
                continue
            destination = target / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            hashes[str(relative)] = hashlib.sha256(path.read_bytes()).hexdigest()
            shutil.copy2(path, destination)
            if path.suffix in {".cpp", ".hpp"}:
                content = destination.read_text().replace("agents/api/agent_api.hpp", "agents/common/api/agent_api.hpp")
                # Historical generated packages named their own files absolutely.
                content = re.sub(r'#include "(?:experiments/[^"\n]+|agents/[^"\n]+)/source/(agent.hpp|tape.inc)"',
                                 r'#include "\1"', content)
                destination.write_text(content)
        (target / "IMPORT.json").write_text(json.dumps({
            "origin": str(origin.relative_to(ROOT)), "source_sha256": hashes,
            "changes": ["Copy into this experiment", "Repair moved includes; policy unchanged"],
            "rating_scope": "Historical source selection band, not a rating measured in this experiment",
        }, indent=2) + "\n")
        catalog[name] = str(target.relative_to(ROOT))
    catalog_path.write_text(json.dumps(catalog, indent=2) + "\n")


if __name__ == "__main__":
    main()
