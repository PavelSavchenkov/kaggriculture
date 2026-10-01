"""Converts extracted tile grids (<dir>/shard_*.grid.f32) to float16 (.grid.f16, as read by
scripts/train.py, like arrays_v5) in chunks, then deletes the float32 file; prints the largest
absolute rounding error.
usage: grid_f16.py <arrays_dir>
"""
import glob
import sys
from pathlib import Path

import numpy as np


def main():
    for path in sorted(glob.glob(f"{sys.argv[1]}/shard_*.grid.f32")):
        source = np.memmap(path, dtype=np.float32, mode="r")
        target = Path(path).with_suffix(".f16")
        error = 0.0
        with open(target, "wb") as out:
            for start in range(0, len(source), 1 << 26):
                chunk = np.asarray(source[start:start + (1 << 26)])
                half = chunk.astype(np.float16)
                error = max(error, float(np.abs(half.astype(np.float32) - chunk).max()))
                half.tofile(out)
        del source
        Path(path).unlink()
        print("converted", target, "max abs error", error)


if __name__ == "__main__":
    main()
