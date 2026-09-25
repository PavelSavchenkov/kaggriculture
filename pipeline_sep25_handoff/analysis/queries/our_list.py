"""Finalizes raw Local-LB traces and writes ours_list.txt for tools/ledger:
line = trace label_seat0 label_seat1 group; our seat is labelled ours:<variant>,
the opponent opp:<name>; group = <variant>/<cpp|lb>/<opponent>."""
import glob
import os
import re
import subprocess
from pathlib import Path

W = Path(__file__).resolve().parent
lines = []
for variant_dir in sorted((W / "games").iterdir()):
    v = variant_dir.name
    for raw in sorted(variant_dir.glob("lb/*.raw")):
        txt = raw.with_suffix(".txt")
        if not txt.exists():
            subprocess.run([str(W / "tools/finalize_trace"), str(raw), str(txt)], check=True)
        m = re.match(r"(.+)_(\d+)_seat(\d)$", raw.stem)
        opp, seat = m.group(1), int(m.group(3))
        labels = [f"ours:{v}", f"opp:{opp}"] if seat == 0 else [f"opp:{opp}", f"ours:{v}"]
        lines.append(f"{txt} {labels[0]} {labels[1]} {v}/lb/{opp}")
    for d in sorted(variant_dir.glob("cpp_*")):
        if not d.is_dir():
            continue
        opp = d.name[4:]
        for t in sorted(d.glob("*.txt")):
            seed, seat = map(int, t.stem.split("_"))
            labels = [f"ours:{v}", f"opp:{opp}"] if seat == 0 else [f"opp:{opp}", f"ours:{v}"]
            if opp == "self":
                labels = [f"ours:{v}", f"self:{v}"] if seat == 0 else [f"self:{v}", f"ours:{v}"]
            lines.append(f"{t} {labels[0]} {labels[1]} {v}/cpp/{opp}")
(W / "ours_list.txt").write_text("\n".join(lines) + "\n")
print(len(lines), "traces")
