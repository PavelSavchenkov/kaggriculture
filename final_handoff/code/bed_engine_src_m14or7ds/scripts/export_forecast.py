"""Export a scripts/train_forecast.py network (forecast.pt) to the binary read by dc11_local/learned_forecast.hpp:
int32 layer count, then per Linear layer int32 rows (out), int32 cols (in), float32 weights (row-major), float32 bias.
usage: scripts/export_forecast.py <forecast.pt> <out file>"""
import struct, sys
import numpy as np, torch
state = torch.load(sys.argv[1], map_location="cpu")
layers = [(state[f"body.{i}.weight"].numpy(), state[f"body.{i}.bias"].numpy()) for i in (0, 2, 4)]
with open(sys.argv[2], "wb") as f:
    f.write(struct.pack("<i", len(layers)))
    for w, b in layers:
        f.write(struct.pack("<ii", *w.shape))
        f.write(np.ascontiguousarray(w, dtype=np.float32).tobytes()); f.write(np.ascontiguousarray(b, dtype=np.float32).tobytes())
print("layers", [w.shape for w, _ in layers])
