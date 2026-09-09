"""Compare literal upstream actions and every frozen routing threshold branch."""
from pathlib import Path
import json
import subprocess
import sys
import argparse

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
sys.path.insert(0,str(EXP/'scripts'))
from generate_public_routes import triple

parser=argparse.ArgumentParser()
parser.add_argument('--check-existing',action='store_true')
args=parser.parse_args()
command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O2','-I',str(ROOT),
    str(RUN/'source/verify.cpp'),str(RUN/'source/policy.cpp'),'-o',str(RUN/'verify')]
if not args.check_existing:
    subprocess.run(command,check=True)
    result=subprocess.run(['conda','run','-n','kaggriculture',str(RUN/'verify')],capture_output=True,text=True,check=True)
    (RUN/'SOURCE_PARITY.txt').write_text(result.stdout)
rows=[[int(x) for x in line.split()] for line in (RUN/'SOURCE_PARITY.txt').read_text().splitlines() if line.strip()]
tapes=json.loads((EXP/'research/refresh_sep08_1108/notebook_audit/yusuke/actions.json.txt').read_text())
for index,action in enumerate(a for tape in tapes for a in tape):
    units=[triple(u or ['PASS']) for u in [action['farmer'],*action['hands']]]
    orders=[triple(o or ['PASS'],True) for o in action['market']]
    assert rows[index]==[len(units),len(orders),*(n for v in units+orders for n in v)],index
for mode,yarn,stock,early,late in rows[2876:]:
    expected_early=yarn if mode else 0
    expected_late=(2 if stock<=9888 else 3) if mode>=2 and (mode==2 or not yarn) else expected_early
    assert (early,late)==(expected_early,expected_late)
assert len(rows)==2900
(RUN/'SOURCE_PARITY.json').write_text(json.dumps({'command':command,'actions':2876,'routing_cases':24,
    'reset_cases':24,'scope':'All four literal action tapes and frozen Yarn/egg thresholds, both threshold sides and equality. Empty upstream slots remain PASS. API worker count is aligned at runtime.'},indent=2)+'\n')
print('Source parity passed:2876actions/24routing cases/24resets',flush=True)
