"""Ledger list for the zoo panel: zoo/<cand>/<model>/<seed>_<seat>.txt, our seat = <seat>."""
import glob
from pathlib import Path
lines = []
for t in sorted(glob.glob('zoo/*/*/*.txt')):
    p = Path(t); cand, model = p.parts[1], p.parts[2]
    seat = int(p.stem.split('_')[1])
    lab = [f'ours:{cand}', f'opp:{model}'] if seat == 0 else [f'opp:{model}', f'ours:{cand}']
    lines.append(f'{t} {lab[0]} {lab[1]} {cand}/{model}')
Path('ledger/zoo_list.txt').write_text('\n'.join(lines) + '\n'); print(len(lines))
