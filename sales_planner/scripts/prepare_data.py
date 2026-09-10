"""Restore selected exposed calendars; generated data stays outside the commit.

Use bundled regression fixtures, existing archive files, or explicitly requested
Kaggle downloads, in that order. Never open the reserved episodes here.
"""
import argparse
import gzip
import hashlib
import inspect
import json
import subprocess
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]
ROOT = PACKAGE.parent


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(episodes, download=False, archive=True):
    manifest = json.loads((PACKAGE / "data/MANIFEST.json").read_text())
    cases = {c["episode"]: c for c in manifest["cases"]}
    # Validate the entire request before reading any replay or creating files.
    for episode in episodes:
        if episode not in cases or cases[episode]["status"] != "exposed":
            raise ValueError(f"Episode {episode} is unknown or reserved; not a development input")
    cache = PACKAGE / "data/cache"
    cache.mkdir(parents=True, exist_ok=True)
    results = []
    for episode in episodes:
        case = cases[episode]
        target = cache / f"{episode}.calendar"
        source = "cache"
        if not target.exists():
            fixture = PACKAGE / case["fixture"] if "fixture" in case else None
            old = ROOT / manifest["archive"] / case["archive_calendar"]
            if fixture is not None and fixture.exists():
                packed = fixture.read_bytes()
                if digest(packed) != case["fixture_sha256"]:
                    raise ValueError(f"Fixture hash mismatch: {fixture}")
                data = gzip.decompress(packed)
                source = "bundled fixture"
            elif archive and old.exists():
                data = old.read_bytes()
                source = "existing archive"
            else:
                raw = PACKAGE / "replays" / f"episode-{episode}-replay.json"
                old_raw = ROOT / manifest["archive"] / case["archive_replay"]
                if not raw.exists() and archive and old_raw.exists():
                    raw = old_raw
                if not raw.exists() and download:
                    raw.parent.mkdir(parents=True, exist_ok=True)
                    subprocess.run(["conda", "run", "-n", "kaggriculture", "kaggle",
                                    "competitions", "replay", str(episode), "-p", str(raw.parent), "-q"], check=True)
                if not raw.exists():
                    raise FileNotFoundError(f"No local data for {episode}; use --download to retrieve it")
                if digest(raw.read_bytes()) != case["replay_sha256"]:
                    raise ValueError(f"Replay hash mismatch: {raw}")
                from extract_calendars import extract, game
                engine_hash = digest(inspect.getsource(game).encode())
                expected = {v["official_engine_sha256"] for v in manifest["extraction"].values()
                            if "official_engine_sha256" in v}
                if engine_hash not in expected:
                    raise ValueError("Official engine changed; revalidate extraction before replacing data")
                result = extract(raw, cache)
                if result["status"] != "extracted":
                    raise ValueError(result)
                data = target.read_bytes()
                source = "replay extraction"
            if digest(data) != case["calendar_sha256"]:
                raise ValueError(f"Calendar hash mismatch for {episode}; retain the failure and inspect versions")
            target.write_bytes(data)
        if digest(target.read_bytes()) != case["calendar_sha256"]:
            raise ValueError(f"Cached calendar hash mismatch: {target}")
        results.append({"episode": episode, "calendar": str(target), "source": source})
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--episodes", type=int, nargs="+")
    group.add_argument("--all-exposed", action="store_true")
    parser.add_argument("--download", action="store_true")
    parser.add_argument("--no-archive", action="store_true")
    args = parser.parse_args()
    manifest = json.loads((PACKAGE / "data/MANIFEST.json").read_text())
    episodes = [c["episode"] for c in manifest["cases"] if c["status"] == "exposed"] if args.all_exposed else (
        args.episodes or [c["episode"] for c in manifest["cases"] if "fixture" in c])
    print(json.dumps(prepare(episodes, args.download, not args.no_archive), indent=2))


if __name__ == "__main__":
    main()
