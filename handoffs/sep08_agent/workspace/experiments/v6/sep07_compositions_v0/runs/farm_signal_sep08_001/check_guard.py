from pathlib import Path
import ast
import copy
import hashlib
import json
import random
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
source = EXP / 'research/refresh_sep08_0708/notebook_audit/farm_signal'
products = ['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON','EGG','MILK','WOOL','FERTILIZER']
items = products + ['GOOSE','COW','SHEEP']
ops = ['PASS','NORTH','SOUTH','EAST','WEST','PICKUP','DROP','PLACE','PLANT','WATER','HARVEST','FERTILIZE','DIG','BUILD_COOP','BUILD_PASTURE','FEED','COLLECT_FERTILIZER','CARE']
markets = ['PASS','HIRE','BUY_LAND','BUY_SEED','BUY_PRODUCT','BUY_ANIMAL','SELL']
schedules, trees = json.loads((source / 'DATA.json').read_text())
tree = ast.parse((source / 'agent_source.py').read_text())
definitions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ['_room_guard','_future_sells']]
assert len(definitions) == 2
namespace = {'PRODUCTS':products, 'SHED_CAP':100, 'SCHEDULES':schedules}
# Execute only the two reviewed, pure oracle functions. Never run the notebook
# or its deployment/agent entry point.
exec(compile(ast.Module(body=definitions, type_ignores=[]), 'reviewed_capacity_oracle', 'exec'), namespace)
rng = random.Random(8307)
fixtures = []
expected = []
changed = 0
for route in range(5):
    for day in range(30):
        for variant in range(12):
            step = min(718, day*24 + (22 if variant == 0 else 23))
            count = rng.randrange(1, 19)
            shed = [0]*12
            for _ in range(rng.randrange(0,101)):
                shed[rng.randrange(12)] += 1
            prices = [100 if variant == 1 else rng.randrange(1,600) for _ in products]
            tiles = [[{} for _ in range(10)] for _ in range(10)]
            positions = [(u%10,u//10) for u in range(count)]
            units, inventories, encoded_units = [], [], []
            for u,(x,y) in enumerate(positions):
                op = rng.choice([0,7,10,11,15,16,17])
                arg = rng.randrange(12)
                n = rng.randrange(1,5)
                unit = [ops[op],items[arg],n]
                units.append(unit)
                inv = [rng.randrange(0,5) if rng.random()<0.12 else 0 for _ in items]
                inventories.append(dict(zip(items,inv)))
                tile = {'yield_units':rng.randrange(0,7),'fertilizer_available':rng.randrange(0,2)}
                tiles[y][x] = tile
                encoded_units += [tile['yield_units'],tile['fertilizer_available'],op,arg,n,*inv]
            market = []
            for j in range(10 if variant in [2,3] else rng.randrange(0,10)):
                op = rng.choice([0,1,3,4,5,6,6,6])
                item = rng.randrange(9) if op==6 else rng.choice([0,8]) if op==4 else rng.randrange(9,12) if op==5 else rng.randrange(5) if op==3 else 0
                market.append([markets[op],items[item],0 if variant == 3 else rng.randrange(0,25)])
            obs = {'player':0, 'farms':[{'farmer':positions[0],'hands':positions[1:],'tiles':tiles}],
                'private':{'shed':dict(zip(items,shed)),'inventories':inventories},
                'market':{'prices':dict(zip(products,prices))}}
            actual = namespace['_room_guard'](obs,step,route,units[0],units[1:],copy.deepcopy(market))
            changed += actual != market
            encode_market = lambda rows: [v for op,item,n in rows for v in [markets.index(op),items.index(item),n]]
            fixtures.append([route,step,count,len(market),*shed,*prices,*encoded_units,*encode_market(market)])
            expected.append([len(actual),*encode_market(actual)])
input_text = str(len(fixtures))+'\n'+'\n'.join(' '.join(map(str,row)) for row in fixtures)+'\n'
(RUN/'guard_fixtures.txt').write_text(input_text)
(RUN/'guard_expected.json').write_text(json.dumps(expected)+'\n')
binary = RUN/'guard_check'
command = ['conda','run','-n','kaggriculture','g++','-std=c++20','-O2','-DNDEBUG','-fno-exceptions','-fno-rtti','-I',str(ROOT),str(RUN/'check_guard.cpp'),'-o',str(binary)]
result = subprocess.run(command,capture_output=True,text=True)
(RUN/'guard_build.log').write_text(result.stdout+result.stderr)
result.check_returncode()
result = subprocess.run(['conda','run','--no-capture-output','-n','kaggriculture',str(binary)],input=input_text,capture_output=True,text=True,check=True)
(RUN/'guard_actual.txt').write_text(result.stdout)
actual = [list(map(int,line.split())) for line in result.stdout.splitlines()]
assert len(actual) == len(expected)
errors = [{'case':i,'actual':a,'expected':e} for i,(a,e) in enumerate(zip(actual,expected)) if a!=e]
report = {'cases':len(fixtures),'changed_cases':changed,'errors':errors,'command':command,
    'source_sha256':hashlib.sha256((source/'agent_source.py').read_bytes()).hexdigest(),
    'fixture_sha256':hashlib.sha256(input_text.encode()).hexdigest(),
    'scope':'Pure capacity decision parity on all five routes, day boundaries, tied prices, full/empty/zero orders, carried stocks and requested service effects. Whole-game source parity and strength are separate.'}
(RUN/'GUARD_PARITY.json').write_text(json.dumps(report,indent=2)+'\n')
assert not errors, errors[:3]
print('Exact guard oracle parity:',len(fixtures),'cases;',changed,'changed market lists.')
