"""Versioned native DayIntent records; no old BC formats are accepted."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np

HEADER = struct.Struct("<QQIHBBhhhhhh")
GLOBAL, PRODUCT, BOARD, GROUP, OPTION = 48, 32, 36, 24, 12


def read_exact(stream, count):
    value = stream.read(count)
    if len(value) != count:
        raise ValueError("Truncated BC export")
    return value


def array(stream, shape, dtype="<f4"):
    dtype = np.dtype(dtype)
    return np.frombuffer(read_exact(stream, int(np.prod(shape)) * dtype.itemsize), dtype=dtype).reshape(shape).copy()


def read_records(path):
    with Path(path).open("rb") as stream:
        if read_exact(stream, 8) != b"BC230001":
            raise ValueError("Unsupported BC data contract")
        while raw := stream.read(HEADER.size):
            identity = HEADER.unpack(raw)
            episode, submission, team, rank, seat, day, nc, na, error, projections, strengthened, missing = identity
            if not (0 <= nc <= 100 and 0 <= na <= 100 and 0 <= day < 30):
                raise ValueError("Invalid native record")
            row = dict(episode=episode, submission=submission, team=team, rank=rank, seat=seat,
                       day=day, error=error, projections=projections, strengthened=strengthened, missing=missing)
            row["global"] = array(stream, (GLOBAL,))
            row["products"] = array(stream, (9, PRODUCT))
            row["board"] = array(stream, (2, 100, BOARD))
            row["globals"] = array(stream, (9,), "<i2")
            row["crops"] = []
            for _ in range(nc):
                key = array(stream, (GROUP,))
                size, options = struct.unpack("<hh", read_exact(stream, 4))
                procedure, maximum, target = [], [], []
                for _ in range(options):
                    procedure.append(array(stream, (OPTION,)))
                    capacity, count = struct.unpack("<hh", read_exact(stream, 4))
                    maximum.append(capacity)
                    target.append(count)
                if sum(target) != size or any(y > m for y, m in zip(target, maximum)):
                    if error == 0:
                        raise ValueError("Compiler accepted an invalid crop partition")
                row["crops"].append((key, size, procedure, maximum, target))
            row["animals"] = []
            for _ in range(na):
                key = array(stream, (GROUP,))
                size, species, forced, serve, escape = struct.unpack("<hhhhh", read_exact(stream, 10))
                if escape != (size - serve if forced else 0) and error == 0:
                    raise ValueError("Animal night partition disagrees with compiler")
                row["animals"].append((key, size, species, forced, serve, escape))
            yield row


def pack(paths, destination, manifest):
    rows = [row for path in paths for row in read_records(path)]
    if not rows:
        raise ValueError("Empty dataset")
    metadata = {(int(r["episode"]), int(r["seat"])): r for r in json.loads(Path(manifest).read_text())}
    nc = max(1, max(len(r["crops"]) for r in rows))
    na = max(1, max(len(r["animals"]) for r in rows))
    no = max(1, max((len(c[2]) for r in rows for c in r["crops"]), default=1))
    n = len(rows)
    data = {name: np.stack([r[name] for r in rows]).astype(np.float32)
            for name in ("global", "products")}
    data["board"] = np.stack([r["board"] for r in rows]).astype(np.float32)
    data["globals"] = np.stack([r["globals"] for r in rows])
    for key in ("episode", "submission", "team", "rank", "seat", "day", "error", "projections", "strengthened", "missing"):
        data[key] = np.array([r[key] for r in rows], dtype=np.int64)
    split_ids = dict(train=0, validation=1, development=2, confirmation=3)
    data["split"] = np.array([split_ids[metadata[r["episode"], r["seat"]]["split"]] for r in rows], np.int8)
    data["crop_key"] = np.zeros((n, nc, GROUP), np.float32)
    data["crop_options"] = np.zeros((n, nc, no, OPTION), np.float32)
    for key, shape in (("crop_size", (n, nc)), ("crop_maximum", (n, nc, no)),
                       ("crop_target", (n, nc, no)), ("animal_size", (n, na)),
                       ("animal_species", (n, na)), ("animal_forced", (n, na)),
                       ("animal_serve", (n, na)), ("animal_escape", (n, na))):
        data[key] = np.zeros(shape, np.int16)
    data["animal_key"] = np.zeros((n, na, GROUP), np.float32)
    for i, row in enumerate(rows):
        for g, (key, size, options, maximum, target) in enumerate(row["crops"]):
            data["crop_key"][i, g] = key
            data["crop_size"][i, g] = size
            data["crop_options"][i, g, :len(options)] = options
            data["crop_maximum"][i, g, :len(options)] = maximum
            data["crop_target"][i, g, :len(options)] = target
        for g, (key, size, species, forced, serve, escape) in enumerate(row["animals"]):
            data["animal_key"][i, g] = key
            for name, value in (("size", size), ("species", species), ("forced", forced), ("serve", serve), ("escape", escape)):
                data[f"animal_{name}"][i, g] = value
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=False)
    for name, value in data.items():
        np.save(destination / f"{name}.npy", value)
    hashes = {}
    for path in sorted(destination.glob("*.npy")):
        with path.open("rb") as stream:
            hashes[path.name] = hashlib.file_digest(stream, "sha256").hexdigest()
    summary = dict(rows=n, perspectives=len({(r["episode"], r["seat"]) for r in rows}),
                   episodes=len({r["episode"] for r in rows}), crop_groups=nc, animal_groups=na, crop_options=no,
                   invalid=int((data["error"] != 0).sum()), projected_dawns=int((data["projections"] > 0).sum()),
                   original_missing_dawns=int((data["missing"] > 0).sum()),
                   splits={name: int((data["split"] == index).sum()) for name, index in split_ids.items()},
                   shapes={name: list(value.shape) for name, value in data.items()}, array_sha256=hashes,
                   data_hash=hashlib.sha256(json.dumps(hashes, sort_keys=True).encode()).hexdigest())
    (destination / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({k: v for k, v in summary.items() if k != "shapes"}, indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--output", required=True)
    parser.add_argument("--manifest", default="data/perspectives.json")
    args = parser.parse_args()
    pack(args.paths, args.output, args.manifest)


if __name__ == "__main__":
    main()
