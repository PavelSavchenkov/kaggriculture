"""Weighted average of model.bin files with identical layouts (fine-tunes of one network: same basin).
usage: soup.py <out dir> <net dir>:<weight> [...]   (weights are normalised; sidecars copied from the first net)"""
import shutil, sys
from pathlib import Path
import numpy as np


def read(path):
    """-> (layout ints per section, float arrays) of train.export's format."""
    raw = open(path, "rb").read()
    pos, ints, floats = 0, [], []

    def take_int(n):
        nonlocal pos
        v = np.frombuffer(raw, np.int32, n, pos); pos += 4 * n; return [int(x) for x in v]

    def take_float(n):
        nonlocal pos
        v = np.frombuffer(raw, np.float32, n, pos).copy(); pos += 4 * n; return v

    count = take_int(1)[0]; ints.append([count])
    if count < 0:
        for _ in range(-count):
            shape = take_int(3); ints.append(shape)
            o, i, k = shape
            floats += [take_float(o * i * k * k), take_float(o)]
        count = take_int(1)[0]; ints.append([count])
    for _ in range(count):
        shape = take_int(2); ints.append(shape)
        floats += [take_float(shape[0] * shape[1]), take_float(shape[0])]
    assert pos == len(raw), f"{path}: {len(raw) - pos} trailing bytes"
    return ints, floats


def write(path, ints, floats):
    """Same order as read: section headers interleaved with (weights, bias) pairs."""
    with open(path, "wb") as f:
        it, fl = iter(ints), iter(floats)
        count = next(it)[0]; np.array([count], np.int32).tofile(f)
        if count < 0:
            for _ in range(-count):
                np.array(next(it), np.int32).tofile(f); next(fl).tofile(f); next(fl).tofile(f)
            count = next(it)[0]; np.array([count], np.int32).tofile(f)
        for _ in range(count):
            np.array(next(it), np.int32).tofile(f); next(fl).tofile(f); next(fl).tofile(f)


out = Path(sys.argv[1]); parts = [a.rsplit(":", 1) for a in sys.argv[2:]]
nets = [(Path(n), float(w)) for n, w in parts]
total = sum(w for _, w in nets)
layout, acc = None, None
for net, w in nets:
    ints, floats = read(net / "model.bin")
    if layout is None:
        layout, acc = ints, [np.zeros_like(x) for x in floats]
    assert ints == layout, f"{net}: layout differs"
    for a, x in zip(acc, floats):
        a += x * (w / total)
out.mkdir(parents=True, exist_ok=True)
write(out / "model.bin", layout, acc)
for side in nets[0][0].glob("model.bin.*"):
    shutil.copy(side, out / side.name)
(out / "SOUP.txt").write_text(" ".join(sys.argv[2:]) + "\n")
print(f"{out}: {len(nets)} nets, {sum(x.size for x in acc)} weights")
