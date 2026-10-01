"""Exports a value model (train_value.py) in the native BC format (train.py export): its first 13
Linear layers and the grid convs have the same layout as the BC network, so the agent's context
code evaluates it; head row 0 = margin / 20,000. Sidecars: .features (from the BC model) and
.condition (strength 150, latest recency, as the agent's inference condition).
usage: export_value.py models/value_v1 models/cand_c2a/model.bin
"""
import shutil
import sys
from pathlib import Path

import torch

import train as bc
import train_value as tv

out, bc_model = Path(sys.argv[1]), sys.argv[2]
net = tv.ValueNet()
net.load_state_dict(torch.load(out / "value.pt", map_location="cpu"))
bc.export(net, out / "value.bin")
shutil.copy(bc_model + ".features", out / "value.bin.features")
shutil.copy(bc_model + ".condition", out / "value.bin.condition")
print("exported", out / "value.bin")
