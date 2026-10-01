"""Export a saved training checkpoint (models/<net>/step<N>.pt) as a playable model folder: model.bin in train.export's format plus the
net's sidecars. train.py exports only the best-validation checkpoint, which is the wrong pick for fine-tunes that shift the data
distribution on purpose. usage: export_step.py <net dir> <step> <out dir>"""
import json, shutil, sys
from pathlib import Path
import torch
sys.path.insert(0, str(Path(__file__).resolve().parent))
import train as T

net_dir, step, out = Path(sys.argv[1]), int(sys.argv[2]), Path(sys.argv[3])
args = json.load(open(net_dir / "history.json"))["args"]
net = T.Net(args["width"], args["grid"], args["marginal"])
net.load_state_dict(torch.load(net_dir / f"step{step}.pt", map_location="cpu"))
out.mkdir(parents=True, exist_ok=True)
T.export(net, out / "model.bin")
for side in net_dir.glob("model.bin.*"):
    shutil.copy(side, out / side.name)
print(f"{out}: step {step} of {net_dir}")
