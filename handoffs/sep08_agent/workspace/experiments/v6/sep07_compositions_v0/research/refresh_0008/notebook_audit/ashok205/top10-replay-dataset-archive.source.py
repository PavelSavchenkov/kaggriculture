from pathlib import Path
from datetime import datetime, timezone
import csv, hashlib, io, json, os, shutil, subprocess, zipfile

import pandas as pd
import pyarrow as pa
import pyarrow.parquet as pq

INDEX_DATASET = "kaggle/kaggriculture-episodes-index"
ARCHIVE_DATASET = "ashok205/kaggriculture-top10-replay-archive"
ARCHIVE_TITLE = "Kaggriculture Daily Top-10 Replay Archive"
TOP_N = 10
HEADER_SCAN_BYTES = 64 * 1024
PUBLISH = True

STAGE = Path("/kaggle/working/kaggriculture_top10_replay_archive")
TMP = Path("/kaggle/working/kaggriculture_top10_replay_tmp")
for p in (STAGE, TMP):
    if p.exists():
        shutil.rmtree(p)
    p.mkdir(parents=True, exist_ok=True)

print({"index": INDEX_DATASET, "archive": ARCHIVE_DATASET, "top_n": TOP_N, "publish": PUBLISH})


#!/usr/bin/env python3
"""Core helpers for the Kaggriculture daily top-10 replay archive."""
from __future__ import annotations

import hashlib
import json
import math
import os
import re
from pathlib import Path
from typing import Any, Iterable, Mapping

SCHEMA_VERSION = 3
_TEAM_NAMES_KEY_RE = re.compile(rb'"TeamNames"\s*:\s*')


def extract_team_names_from_prefix(prefix: bytes | str) -> tuple[str, ...]:
    """Extract replay info.TeamNames from a small JSON prefix.

    Kaggriculture replay files place TeamNames in the header before the large steps
    array, so reading only the first few KiB is sufficient for participant discovery.
    """
    raw = prefix.encode("utf-8") if isinstance(prefix, str) else bytes(prefix)
    match = _TEAM_NAMES_KEY_RE.search(raw)
    if match is None:
        raise RuntimeError("replay prefix does not contain TeamNames")
    try:
        # Decode from the value start and let the JSON parser determine where the
        # array ends. A regex cannot safely find the closing bracket because `]`
        # is legal inside a quoted team name. Ignore only a potentially truncated
        # UTF-8 code point at the far end of the replay prefix.
        tail = raw[match.end():].decode("utf-8", errors="ignore")
        names, _ = json.JSONDecoder().raw_decode(tail)
    except Exception as exc:
        raise RuntimeError(f"invalid TeamNames header: {exc}") from exc
    if not isinstance(names, list) or len(names) < 2 or any(not isinstance(x, str) or not x for x in names):
        raise RuntimeError(f"invalid TeamNames value: {names!r}")
    return tuple(names)


def replay_entry_map(zf) -> dict[int, Any]:
    """Map numeric replay JSON filenames to ZIP members, keeping the first duplicate."""
    out: dict[int, Any] = {}
    for info in zf.infolist():
        name = Path(info.filename).name
        if not name.endswith(".json"):
            continue
        stem = name[:-5]
        if stem.isdigit():
            out.setdefault(int(stem), info)
    return out


def scan_team_names_from_zip(
    zf,
    manifest_rows: Iterable[Mapping[str, Any]],
    *,
    header_scan_bytes: int = 64 * 1024,
) -> tuple[dict[int, Any], dict[int, tuple[str, ...]], list[dict[str, Any]]]:
    """Scan replay headers, skipping source rows whose replay member is unavailable/bad."""
    entries = replay_entry_map(zf)
    names_by_episode: dict[int, tuple[str, ...]] = {}
    skipped: list[dict[str, Any]] = []
    for row in manifest_rows:
        try:
            episode_id = int(row["episode_id"])
        except Exception:
            continue
        info = entries.get(episode_id)
        if info is None:
            skipped.append({"episode_id": episode_id, "reason": "missing_member"})
            continue
        try:
            with zf.open(info) as f:
                prefix = f.read(int(header_scan_bytes))
        except Exception:
            skipped.append({"episode_id": episode_id, "reason": "unreadable_header"})
            continue
        try:
            names_by_episode[episode_id] = extract_team_names_from_prefix(prefix)
        except Exception:
            skipped.append({"episode_id": episode_id, "reason": "invalid_header"})
    return entries, names_by_episode, skipped


def read_replay_bytes_from_zip(
    zf,
    entries: Mapping[int, Any],
    episode_id: int,
) -> tuple[bytes | None, str | None]:
    """Read a selected replay body without letting one bad ZIP member abort the day."""
    episode_id = int(episode_id)
    info = entries.get(episode_id)
    if info is None:
        return None, "missing_member"
    try:
        raw = zf.read(info)
    except Exception:
        return None, "unreadable_member"
    try:
        raw.decode("utf-8")
    except UnicodeDecodeError:
        return None, "invalid_utf8"
    return raw, None


def rank_daily_teams(
    manifest_rows: Iterable[Mapping[str, Any]],
    team_names_by_episode: Mapping[int, Iterable[str]],
    n: int = 10,
) -> list[dict[str, Any]]:
    """Rank teams from official daily replay score evidence.

    The daily manifest gives the exact pairwise post-episode rating evidence as
    avg_score/min_score.  We do not assume replay ordering is a player ranking.
    Instead, every team is discovered from replay headers and ranked by its strongest
    observed average-agent-rating episode that day.  min_score and the implied upper
    agent score are deterministic tie-break/evidence fields.
    """
    if n < 1:
        raise ValueError("n must be positive")
    stats: dict[str, dict[str, Any]] = {}
    seen_episode_ids: set[int] = set()
    for raw in manifest_rows:
        episode_id = int(raw["episode_id"])
        if episode_id in seen_episode_ids:
            raise RuntimeError(f"duplicate episode id in daily manifest: {episode_id}")
        seen_episode_ids.add(episode_id)
        if episode_id not in team_names_by_episode:
            continue
        try:
            avg_score = float(raw["avg_score"])
            min_score = float(raw["min_score"])
        except (KeyError, TypeError, ValueError):
            continue
        if not math.isfinite(avg_score) or not math.isfinite(min_score):
            continue
        upper_score = 2.0 * avg_score - min_score
        names = tuple(dict.fromkeys(str(x) for x in team_names_by_episode[episode_id] if str(x)))
        if not names:
            continue
        for name in names:
            row = stats.setdefault(
                name,
                {
                    "team_name": name,
                    "appearances": 0,
                    "score_proxy": float("-inf"),
                    "max_min_score": float("-inf"),
                    "max_agent_score_upper": float("-inf"),
                    "evidence_episode_id": None,
                },
            )
            row["appearances"] += 1
            candidate = (avg_score, min_score, upper_score, -episode_id)
            incumbent = (
                float(row["score_proxy"]),
                float(row["max_min_score"]),
                float(row["max_agent_score_upper"]),
                -int(row["evidence_episode_id"]) if row["evidence_episode_id"] is not None else float("-inf"),
            )
            if candidate > incumbent:
                row["score_proxy"] = avg_score
                row["max_min_score"] = min_score
                row["max_agent_score_upper"] = upper_score
                row["evidence_episode_id"] = episode_id

    ranked = sorted(
        stats.values(),
        key=lambda r: (
            -float(r["score_proxy"]),
            -float(r["max_min_score"]),
            -float(r["max_agent_score_upper"]),
            str(r["team_name"]),
        ),
    )
    out: list[dict[str, Any]] = []
    for rank, row in enumerate(ranked[:n], 1):
        item = dict(row)
        item["rank"] = rank
        item["ranking_basis"] = "official daily replay score evidence: max observed avg_score by team"
        out.append(item)
    return out


def select_episode_ids_by_team_names(
    team_names_by_episode: Mapping[int, Iterable[str]],
    selected_team_names: set[str],
) -> list[int]:
    selected: list[int] = []
    for episode_id, names in team_names_by_episode.items():
        if selected_team_names.intersection(str(x) for x in names):
            selected.append(int(episode_id))
    return sorted(set(selected))


def source_row_digest(row: Mapping[str, Any]) -> str:
    canonical = {str(k): "" if v is None else str(v) for k, v in sorted(row.items(), key=lambda kv: str(kv[0]))}
    payload = json.dumps(canonical, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def pending_source_days(index_rows: Iterable[Mapping[str, Any]], processed: Mapping[str, Any]) -> list[dict[str, Any]]:
    pending: list[dict[str, Any]] = []
    for raw in sorted(index_rows, key=lambda r: str(r["date"])):
        row = dict(raw)
        date = str(row["date"])
        digest = source_row_digest(row)
        prev = processed.get(date)
        if prev is None or prev.get("status") != "complete":
            pending.append(row)
            continue
        if str(prev.get("source_row_sha256")) != digest:
            raise RuntimeError(f"source index row for {date} changed after processing")
    return pending


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def atomic_write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = (json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(",", ":")) + "\n").encode("utf-8")
    tmp = path.with_name(path.name + ".tmp")
    with tmp.open("wb") as f:
        f.write(payload)
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, path)


def _manifest_payload(manifest: Mapping[str, Any]) -> bytes:
    body = {k: v for k, v in manifest.items() if k != "manifest_sha256"}
    return json.dumps(body, sort_keys=True, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def write_archive_manifest(
    root: Path,
    *,
    processed_dates: list[str],
    episode_count: int,
    top10_row_count: int,
    source_index_manifest_sha256: str,
) -> dict[str, Any]:
    files: dict[str, Any] = {}
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.name.endswith(".tmp") or path.name in {"archive_manifest.json", "dataset-metadata.json"}:
            continue
        rel = path.relative_to(root).as_posix()
        files[rel] = {"sha256": sha256_file(path), "bytes": path.stat().st_size}
    manifest: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "source_index": "kaggle/kaggriculture-episodes-index",
        "ranking_source": "official daily replay ZIP headers + manifest score evidence",
        "processed_dates": sorted(processed_dates),
        "episode_count": int(episode_count),
        "top10_row_count": int(top10_row_count),
        "source_index_manifest_sha256": source_index_manifest_sha256,
        "files": files,
    }
    manifest["manifest_sha256"] = hashlib.sha256(_manifest_payload(manifest)).hexdigest()
    atomic_write_json(root / "archive_manifest.json", manifest)
    return manifest


def validate_archive(root: Path) -> dict[str, Any]:
    path = root / "archive_manifest.json"
    if not path.exists():
        raise RuntimeError(f"archive manifest missing: {path}")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if int(manifest.get("schema_version", -1)) != SCHEMA_VERSION:
        raise RuntimeError(f"archive schema mismatch: expected {SCHEMA_VERSION}, got {manifest.get('schema_version')}")
    expected_manifest_sha = str(manifest.get("manifest_sha256", ""))
    actual_manifest_sha = hashlib.sha256(_manifest_payload(manifest)).hexdigest()
    if expected_manifest_sha != actual_manifest_sha:
        raise RuntimeError("archive manifest hash mismatch")
    for rel, meta in (manifest.get("files") or {}).items():
        file_path = root / rel
        if not file_path.exists():
            raise RuntimeError(f"archive file missing: {rel}")
        if sha256_file(file_path) != str(meta["sha256"]):
            raise RuntimeError(f"archive file hash mismatch: {rel}")
        if file_path.stat().st_size != int(meta["bytes"]):
            raise RuntimeError(f"archive file size mismatch: {rel}")
    return manifest


def run(cmd, *, timeout=7200):
    print("$", " ".join(map(str, cmd)), flush=True)
    p = subprocess.run([str(x) for x in cmd], text=True, capture_output=True, timeout=timeout)
    if p.returncode != 0:
        raise RuntimeError(f"command failed ({p.returncode}): {' '.join(map(str, cmd))}\n{p.stdout}\n{p.stderr}")
    if p.stdout.strip():
        print(p.stdout.strip())
    if p.stderr.strip():
        print(p.stderr.strip())
    return p


def locate_index_manifest():
    path = Path("/kaggle/input/datasets/kaggle/kaggriculture-episodes-index/manifest.csv")
    if not path.exists():
        raise RuntimeError(
            "kaggle/kaggriculture-episodes-index is not attached as a notebook input; "
            f"expected {path}"
        )
    return path


def locate_archive_input():
    candidates = [
        Path("/kaggle/input/datasets/ashok205/kaggriculture-top10-replay-archive"),
        Path("/kaggle/input/kaggriculture-top10-replay-archive"),
    ]
    for p in candidates:
        if (p / "archive_manifest.json").exists():
            return p
    hits = [p.parent for p in Path("/kaggle/input").rglob("archive_manifest.json") if "top10" in str(p).lower()]
    if len(hits) > 1:
        raise RuntimeError(f"ambiguous prior archive mounts: {hits}")
    return hits[0] if hits else None


def csv_rows(path):
    with Path(path).open(encoding="utf-8-sig", newline="") as f:
        return [dict(r) for r in csv.DictReader(f)]


def csv_rows_from_bytes(raw):
    text = raw.decode("utf-8-sig")
    return [dict(r) for r in csv.DictReader(io.StringIO(text))]


def read_parquet_rows(path):
    if not Path(path).exists():
        return []
    return pd.read_parquet(path).to_dict("records")


def write_rows_parquet_atomic(path, rows, sort_cols):
    path = Path(path)
    tmp = path.with_name(path.name + ".tmp")
    df = pd.DataFrame(rows)
    if len(df):
        df = df.sort_values(sort_cols, kind="mergesort").reset_index(drop=True)
    df.to_parquet(tmp, index=False, compression="zstd")
    os.replace(tmp, path)


def download_daily_zip(slug, date):
    day_tmp = TMP / f"day_{date}"
    if day_tmp.exists():
        shutil.rmtree(day_tmp)
    day_tmp.mkdir(parents=True)
    run(["kaggle", "datasets", "download", f"kaggle/{slug}", "-p", day_tmp, "-o", "-q"])
    zips = list(day_tmp.glob("*.zip"))
    if len(zips) != 1:
        raise RuntimeError(f"expected one daily ZIP for {date}, found {zips}")
    if zips[0].stat().st_size <= 0:
        raise RuntimeError(f"empty daily ZIP for {date}")
    return day_tmp, zips[0]


def optional_float(value):
    try:
        x = float(value)
    except (TypeError, ValueError):
        return None
    return x if math.isfinite(x) else None


def write_replay_shard_from_zip(date, zf, entries, selected_ids):
    final = STAGE / f"replays_{date}.parquet"
    if final.exists():
        raise RuntimeError(f"day shard already exists for unprocessed day: {final.name}")
    tmp = TMP / f"replays_{date}.parquet"
    schema = pa.schema([("episode_id", pa.int64()), ("replay_json", pa.large_string())])
    writer = pq.ParquetWriter(tmp, schema, compression="zstd", compression_level=10)
    replay_hashes = {}
    archived_ids = []
    body_skips = []
    ordered = sorted(set(map(int, selected_ids)))
    try:
        for i, episode_id in enumerate(ordered, 1):
            raw, reason = read_replay_bytes_from_zip(zf, entries, episode_id)
            if raw is None:
                body_skips.append({"episode_id": episode_id, "reason": reason or "unreadable_member"})
                continue
            replay_hashes[episode_id] = hashlib.sha256(raw).hexdigest()
            writer.write_table(pa.Table.from_arrays([
                pa.array([episode_id], type=pa.int64()),
                pa.array([raw.decode("utf-8")], type=pa.large_string()),
            ], schema=schema))
            archived_ids.append(episode_id)
            if i % 25 == 0:
                print(f"  stored replays {i}/{len(ordered)}", flush=True)
    finally:
        writer.close()
    if not archived_ids:
        tmp.unlink(missing_ok=True)
        raise RuntimeError(f"no readable selected replay bodies remain for {date}")
    os.replace(tmp, final)
    check = pq.read_table(final, columns=["episode_id"]).column("episode_id").to_pylist()
    if check != archived_ids:
        raise RuntimeError(f"day shard verification failed for {date}")
    return final, replay_hashes, archived_ids, body_skips


# Tiny deterministic smoke test before touching Kaggle sources.
_smoke_rows = [
    {"episode_id": "3", "avg_score": "1000", "min_score": "900"},
    {"episode_id": "1", "avg_score": "1300", "min_score": "1250"},
    {"episode_id": "2", "avg_score": "1200", "min_score": "1180"},
]
_smoke_names = {1: ("A", "B"), 2: ("C", "D"), 3: ("E", "F")}
_smoke_top = rank_daily_teams(_smoke_rows, _smoke_names, 4)
assert [r["team_name"] for r in _smoke_top] == ["A", "B", "C", "D"]
assert select_episode_ids_by_team_names(_smoke_names, {"A"}) == [1]
assert extract_team_names_from_prefix(b'{"info":{"TeamNames":["A","B"]},"steps":[') == ("A", "B")
print("PURE SMOKE PASSED")


# Load the previous published archive if it exists.
prior = locate_archive_input()
if prior is not None:
    prior_manifest = validate_archive(prior)
    for p in prior.iterdir():
        if p.is_file():
            shutil.copy2(p, STAGE / p.name)
    print("validated prior archive", prior, "dates", len(prior_manifest.get("processed_dates", [])))
else:
    print("no prior archive manifest: first-run historical bootstrap")

processed_path = STAGE / "processed_days.json"
top10_path = STAGE / "top10_history.parquet"
episodes_path = STAGE / "episodes.parquet"
processed = json.loads(processed_path.read_text()) if processed_path.exists() else {}
top10_rows = read_parquet_rows(top10_path)
episode_rows = read_parquet_rows(episodes_path)

index_path = locate_index_manifest()
index_rows = csv_rows(index_path)
index_sha256 = sha256_file(index_path)
pending = pending_source_days(index_rows, processed)
print(f"official index: {len(index_rows)} days sha256={index_sha256}")
print("pending dates:", [r["date"] for r in pending])


processed_this_run = []

for source_row in pending:
    date = str(source_row["date"])
    slug = str(source_row["daily_dataset_slug"])
    print(f"\n=== {date} {slug} ===", flush=True)
    day_tmp, zip_path = download_daily_zip(slug, date)
    zip_bytes = zip_path.stat().st_size

    try:
        with zipfile.ZipFile(zip_path) as zf:
            if "manifest.csv" not in zf.namelist():
                raise RuntimeError(f"daily ZIP {date} has no manifest.csv")
            daily_manifest_raw = zf.read("manifest.csv")
            daily_manifest_sha = sha256_bytes(daily_manifest_raw)
            raw_daily_rows = csv_rows_from_bytes(daily_manifest_raw)
            expected_count = int(source_row["episode_count"])
            manifest_count_mismatch = len(raw_daily_rows) != expected_count

            # Be tolerant of isolated malformed/duplicate source rows. Keep the first
            # valid row for each episode id and record only aggregate skip counts.
            daily_rows = []
            seen_manifest_ids = set()
            invalid_manifest_row_count = 0
            duplicate_manifest_row_count = 0
            for row in raw_daily_rows:
                try:
                    episode_id = int(row["episode_id"])
                except Exception:
                    invalid_manifest_row_count += 1
                    continue
                if episode_id in seen_manifest_ids:
                    duplicate_manifest_row_count += 1
                    continue
                seen_manifest_ids.add(episode_id)
                daily_rows.append(row)

            entries, names_by_episode, header_skips = scan_team_names_from_zip(
                zf, daily_rows, header_scan_bytes=HEADER_SCAN_BYTES
            )
            if header_skips:
                reason_counts = {}
                for item in header_skips:
                    reason_counts[item["reason"]] = reason_counts.get(item["reason"], 0) + 1
                print(f"  skipped {len(header_skips)} unavailable/bad replay headers: {reason_counts}")
            top = rank_daily_teams(daily_rows, names_by_episode, TOP_N)
            if len(top) != TOP_N:
                raise RuntimeError(f"only {len(top)} teams found in {date}; refusing partial top-10")
            rank_by_name = {str(r["team_name"]): int(r["rank"]) for r in top}
            top_by_name = {str(r["team_name"]): r for r in top}
            top_names = set(rank_by_name)
            selected_ids = select_episode_ids_by_team_names(names_by_episode, top_names)
            if not selected_ids:
                raise RuntimeError(f"no top-10 episodes selected for {date}")

            print("top10:", [(r["rank"], r["team_name"], round(float(r["score_proxy"]), 1)) for r in top])
            print(f"selected {len(selected_ids)}/{len(daily_rows)} source episodes")

            shard, replay_hashes, archived_ids, body_skips = write_replay_shard_from_zip(
                date, zf, entries, selected_ids
            )
            if body_skips:
                reason_counts = {}
                for item in body_skips:
                    reason_counts[item["reason"]] = reason_counts.get(item["reason"], 0) + 1
                print(f"  skipped {len(body_skips)} unreadable selected replay bodies: {reason_counts}")

            manifest_by_id = {int(r["episode_id"]): r for r in daily_rows}
            incoming_episode_rows = []
            for episode_id in archived_ids:
                participants = tuple(names_by_episode[episode_id])
                matched = list(dict.fromkeys(name for name in participants if name in top_names))
                sm = manifest_by_id[episode_id]
                avg_score = optional_float(sm.get("avg_score"))
                min_score = optional_float(sm.get("min_score"))
                upper_score = None if avg_score is None or min_score is None else 2.0 * avg_score - min_score
                for team_name in matched:
                    opponents = [name for name in participants if name != team_name]
                    evidence = top_by_name[team_name]
                    incoming_episode_rows.append({
                        "date": date,
                        "episode_id": episode_id,
                        "team_name": team_name,
                        "daily_rank": rank_by_name[team_name],
                        "daily_score_proxy": float(evidence["score_proxy"]),
                        "opponent": " | ".join(opponents),
                        "participants_json": json.dumps(list(participants), ensure_ascii=False, separators=(",", ":")),
                        "source_daily_dataset_slug": slug,
                        "source_index_row_sha256": source_row_digest(source_row),
                        "source_daily_manifest_sha256": daily_manifest_sha,
                        "source_avg_score": avg_score,
                        "source_min_score": min_score,
                        "source_agent_score_upper": upper_score,
                        "source_sum_score": optional_float(sm.get("sum_score")),
                        "replay_sha256": replay_hashes[episode_id],
                        "replay_shard": shard.name,
                    })

            incoming_top = [dict(r, date=date) for r in top]
            top10_rows.extend(incoming_top)
            episode_rows.extend(incoming_episode_rows)

            top_keys = [(str(r["date"]), int(r["rank"])) for r in top10_rows]
            if len(top_keys) != len(set(top_keys)):
                raise RuntimeError("duplicate date/rank in top10 history")
            ep_keys = [(str(r["date"]), int(r["episode_id"]), str(r["team_name"])) for r in episode_rows]
            if len(ep_keys) != len(set(ep_keys)):
                raise RuntimeError("duplicate date/episode/team metadata row")

            processed[date] = {
                "status": "complete",
                "source_row_sha256": source_row_digest(source_row),
                "daily_dataset_slug": slug,
                "daily_manifest_sha256": daily_manifest_sha,
                "daily_manifest_episode_count": len(raw_daily_rows),
                "usable_manifest_episode_count": len(daily_rows),
                "manifest_count_mismatch": bool(manifest_count_mismatch),
                "invalid_manifest_row_count": int(invalid_manifest_row_count),
                "duplicate_manifest_row_count": int(duplicate_manifest_row_count),
                "source_unavailable_episode_count": len(header_skips),
                "source_unavailable_episode_reasons": {
                    reason: sum(1 for item in header_skips if item["reason"] == reason)
                    for reason in sorted({item["reason"] for item in header_skips})
                },
                "candidate_selected_episode_count": len(selected_ids),
                "selected_episode_count": len(archived_ids),
                "selected_unavailable_episode_count": len(body_skips),
                "selected_unavailable_episode_reasons": {
                    reason: sum(1 for item in body_skips if item["reason"] == reason)
                    for reason in sorted({item["reason"] for item in body_skips})
                },
                "ranking_basis": "official daily replay score evidence: max observed avg_score by team",
                "temporary_zip_bytes": int(zip_bytes),
                "replay_shard": shard.name,
                "replay_shard_sha256": sha256_file(shard),
            }

        write_rows_parquet_atomic(top10_path, top10_rows, ["date", "rank"])
        write_rows_parquet_atomic(episodes_path, episode_rows, ["date", "episode_id", "team_name"])
        atomic_write_json(processed_path, processed)
        processed_this_run.append(date)
    finally:
        shutil.rmtree(day_tmp, ignore_errors=True)

metadata = {
    "title": ARCHIVE_TITLE,
    "id": ARCHIVE_DATASET,
    "licenses": [{"name": "CC0-1.0"}],
    "subtitle": "Compact daily Kaggriculture top-10 replay corpus with deterministic provenance",
    "description": "Maintained from the official Kaggriculture Episodes Index and daily replay datasets. Player selection is derived from replay TeamNames plus official daily score evidence; daily file order is never used as a player ranking.",
}
(STAGE / "dataset-metadata.json").write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

manifest = write_archive_manifest(
    STAGE,
    processed_dates=sorted(processed),
    episode_count=len({int(r["episode_id"]) for r in episode_rows}),
    top10_row_count=len(top10_rows),
    source_index_manifest_sha256=index_sha256,
)
validate_archive(STAGE)
print("ARCHIVE VALIDATED", {"dates": len(processed), "episodes": manifest["episode_count"], "new_dates": processed_this_run})


!pip install -U kaggle

# Publish the already-built archive in STAGE.
# First publication creates the dataset; later publications create a new version.

if PUBLISH and processed_this_run:
    msg = f"top10 archive through {max(processed_this_run)}; +{len(processed_this_run)} day(s)"

    # Check whether our target dataset already exists.
    status = subprocess.run(
        ["kaggle", "datasets", "status", ARCHIVE_DATASET],
        text=True,
        capture_output=True,
    )

    if status.returncode == 0:
        print("Remote dataset exists; creating new version:", ARCHIVE_DATASET)
        run([
            "kaggle", "datasets", "version",
            "-p", STAGE,
            "-m", msg,
            "-q",
            "-t",
            "-r", "skip",
        ])
        print("PUBLISHED VERSION", ARCHIVE_DATASET, msg)

    else:
        print("Remote dataset does not exist yet; creating it:", ARCHIVE_DATASET)
        run([
            "kaggle", "datasets", "create",
            "-p", STAGE,
            "-q",
            "-t",
            "-r", "skip",
        ])
        print("CREATED DATASET", ARCHIVE_DATASET)

elif not processed_this_run:
    # Useful when rerunning only this cell after a failed publish:
    # STAGE already contains the completed archive, so publish it anyway.
    msg = "publish completed top10 replay archive"

    status = subprocess.run(
        ["kaggle", "datasets", "status", ARCHIVE_DATASET],
        text=True,
        capture_output=True,
    )

    if status.returncode == 0:
        run([
            "kaggle", "datasets", "version",
            "-p", STAGE,
            "-m", msg,
            "-q",
            "-t",
            "-r", "skip",
        ])
        print("PUBLISHED VERSION", ARCHIVE_DATASET)
    else:
        run([
            "kaggle", "datasets", "create",
            "-p", STAGE,
            "-q",
            "-t",
            "-r", "skip",
        ])
        print("CREATED DATASET", ARCHIVE_DATASET)

else:
    print("PUBLISH=False: validated archive left in", STAGE)

print(json.dumps({
    "archive_dataset": ARCHIVE_DATASET,
    "processed_dates_total": len(processed),
    "processed_dates_this_run": processed_this_run,
    "unique_archived_episodes": len({int(r["episode_id"]) for r in episode_rows}),
    "top10_rows": len(top10_rows),
    "archive_manifest_sha256": manifest["manifest_sha256"],
    "working_archive_bytes": sum(p.stat().st_size for p in STAGE.iterdir() if p.is_file()),
}, indent=2))


