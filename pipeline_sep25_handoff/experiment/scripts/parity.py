"""Compare native global-head logits (tools/parity) with PyTorch on the extracted arrays."""
import sys
import numpy as np
import torch
sys.path.insert(0, "scripts")
from train import Net, Batch, load

native = [line.split() for line in open(sys.argv[1])]
d = load(sys.argv[3] if len(sys.argv) > 3 else "data/arrays_v1")
key = {(int(m[0]), int(m[1]), int(m[2])): i for i, m in enumerate(d["meta"])}
state = torch.load(sys.argv[2], map_location="cpu")
net = Net(state["g_enc.0.weight"].shape[0], "grid.conv1.weight" in state)
for head in ("c_head", "a_head"):  # older checkpoints have fewer head outputs
    weight = state[f"{head}.4.weight"]
    getattr(net, head)[4] = torch.nn.Linear(weight.shape[1], weight.shape[0])
net.load_state_dict(state)
net.eval()
worst, argmax_same = 0.0, 0
for row in native:
    i = key[(int(row[0]), int(row[1]), int(row[2]))]
    with torch.no_grad():
        og, _, _ = net(Batch(d, np.array([i]), "cpu"))
    torch_logits = og[0].numpy()
    native_logits = np.array(row[3:], dtype=np.float64)
    worst = max(worst, float(np.abs(torch_logits - native_logits).max()))
    same = all(np.argmax(torch_logits[f * 101:(f + 1) * 101]) == np.argmax(native_logits[f * 101:(f + 1) * 101]) for f in range(11))
    argmax_same += same and (torch_logits[11 * 101] > 0) == (native_logits[11 * 101] > 0)
print(f"dawns {len(native)} max abs logit error {worst:.2e} identical global decisions {argmax_same}/{len(native)}")
