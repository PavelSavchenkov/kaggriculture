"""Converts an fp32 network file (train.py export: optional conv section, then Linear layers) to the fp16 layout read by the
patched Model::load (scripts/fp16_loader_patch.py): int32 magic 0x36314648, then the same headers with float16 weights and biases.
usage: fp16_model.py <in model.bin> <out model.bin>"""
import sys
import numpy as np

MAGIC = 0x36314648
src, dst = sys.argv[1], sys.argv[2]
data = open(src, "rb").read()
at = 0


def ints(n):
    global at
    out = np.frombuffer(data, np.int32, n, at)
    at += 4 * n
    return out


def floats(n):
    global at
    out = np.frombuffer(data, np.float32, n, at)
    at += 4 * n
    return out


parts = [np.array([MAGIC], np.int32).tobytes()]
n = int(ints(1)[0])
if n < 0:
    parts.append(np.array([n], np.int32).tobytes())
    for _ in range(-n):
        out, cin, k = (int(v) for v in ints(3))
        parts += [np.array([out, cin, k], np.int32).tobytes(), floats(out * cin * k * k).astype(np.float16).tobytes(),
                  floats(out).astype(np.float16).tobytes()]
    n = int(ints(1)[0])
parts.append(np.array([n], np.int32).tobytes())
worst = 0.0
for _ in range(n):
    rows, cols = (int(v) for v in ints(2))
    w, b = floats(rows * cols), floats(rows)
    worst = max(worst, float(np.abs(w.astype(np.float16).astype(np.float32) - w).max()), float(np.abs(w).max()) / 65504)
    parts += [np.array([rows, cols], np.int32).tobytes(), w.astype(np.float16).tobytes(), b.astype(np.float16).tobytes()]
if at != len(data):
    raise SystemExit(f"{src}: {len(data) - at} trailing bytes; unexpected layout")
open(dst, "wb").write(b"".join(parts))
print(f"{src} -> {dst}: {len(data) / 1e6:.1f} MB -> {sum(len(p) for p in parts) / 1e6:.1f} MB, max abs weight rounding {worst:.2e}")
