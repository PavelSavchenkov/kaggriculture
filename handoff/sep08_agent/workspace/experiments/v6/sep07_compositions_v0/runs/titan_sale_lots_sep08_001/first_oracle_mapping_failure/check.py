"""Compare C++ lots with explicitly selected, reviewed pure source functions."""
from datetime import datetime, timezone
from functools import lru_cache
from pathlib import Path
from types import SimpleNamespace
import ast
import hashlib
import json
import math
import random
import subprocess
import time

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
SOURCE = EXP / 'research/refresh_sep08_0608/notebook_audit/tokenjunkie/release'


def definitions(path, functions, constants, namespace):
    tree = ast.parse(path.read_text())
    chosen = [node for node in tree.body if
        isinstance(node, (ast.FunctionDef, ast.ClassDef)) and node.name in functions or
        isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id in constants for t in node.targets)]
    assert {n.name for n in chosen if isinstance(n, (ast.FunctionDef, ast.ClassDef))} == set(functions)
    # No source imports, entrypoints, loaders, network calls or notebooks execute.
    exec(compile(ast.Module(body=chosen, type_ignores=[]), str(path), 'exec'), namespace)
    return namespace


mechanics = definitions(SOURCE/'mechanics.py', ['_shape', '_resolve_market_params', 'market_price'],
    ['MARKET_I0', 'PRICE_FLOOR', 'MARKET_PARAMS', 'HINGE_GAIN', 'SHOPS'], {'math': math})
receipt = definitions(SOURCE/'reference/decision/decision.py', ['_number', 'sale_receipts'], [], {'isfinite': math.isfinite})
oracle = definitions(SOURCE/'scheduler.py', ['absorption', 'MarketPath', 'optimize_lot'], [],
    {'m': SimpleNamespace(**mechanics), 'receipt_math': SimpleNamespace(**receipt), 'lru_cache': lru_cache})
items = ['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON','EGG','MILK','WOOL','FERTILIZER']
shops = ['FARMERS_MARKET','BAKERY','PIZZA_SHOP','PET_CAFE','YARN_STORE']
# Use the exact C++ enum ordering, not dictionary insertion order.
shops = ['FARMERS_MARKET','BAKERY','PIZZA_SHOP','PET_CAFE','BRUNCH_SPOT','ICE_CREAM_SHOP','SMOOTHIE_SHOP','YARN_STORE']
assert set(shops) == set(mechanics['SHOPS'])
rng = random.Random(603808)
cases = []
for index in range(192):
    now = rng.choice([216, 230, 270, 598, 710])
    offsets = rng.choice([[0], [0,1], [0,1,3], [0,1,3,8]])
    quantity = rng.choice([0,1,2,8,16,32,64,100])
    first = rng.randrange(quantity+1)
    dates = [now+d for d in offsets]
    reference = [(now,first)]
    if len(dates)>1 and rng.randrange(2):reference.append((dates[-1],quantity-first))
    cases.append({'item': rng.randrange(9), 'quantity': quantity, 'inventory': rng.choice([9500,9900,9999,10000,10058,10062,10066,10100,10500]),
        'now': now, 'last': 718, 'rival_quantity': rng.choice([0,1,3,11,32,100]), 'minimum_now': rng.randrange(quantity+1),
        'dates': dates, 'shops': [rng.randrange(8) for _ in range(rng.randrange(9))], 'reference': reference,
        'capacity_minimum_now': rng.choice([0,first,quantity])})
for item, inventory in [(3,10062),(6,10076),(7,10058),(4,10156)]:
    for quantity in [1,11,100]:
        for now in [598,710]:
            cases.append({'item':item,'quantity':quantity,'inventory':inventory,'now':now,'last':718,'rival_quantity':11,
                'minimum_now':0,'dates':[now,now+1,now+3,now+8],'shops':[6,7],
                'reference':[(now,quantity)],'capacity_minimum_now':0})
expected = []
lines = [str(len(cases))]
start = time.perf_counter()
for c in cases:
    plan, info = oracle['optimize_lot'](item=items[c['item']],quantity=c['quantity'],inventory=c['inventory'],params=None,
        shops=[shops[s] for s in c['shops']],config={},now=c['now'],dates=c['dates'],reference=tuple(map(tuple,c['reference'])),
        rival_quantity=c['rival_quantity'],minimum_now=c['minimum_now'],capacity_ok=lambda p,c=c:dict(p).get(c['now'],0)>=c['capacity_minimum_now'],last=c['last'])
    scores = list(info['scenarios'].values())
    expected.append({'plan':list(map(list,plan)),'worst_gain':info['worst_relative_gain'],
        'sum_gain':sum(s['relative_value']-s['reference_relative_value'] for s in scores),
        'plans':info['plans_evaluated'],'scores':[[s['relative_value'],s['own_receipts'],s['rival_receipts'],s['carry_units']] for s in scores]})
    values = [c[k] for k in ['item','quantity','inventory','now','last','rival_quantity','minimum_now']]
    values += [len(c['dates']),*c['dates'],len(c['shops']),*c['shops'],len(c['reference'])]
    values += [v for row in c['reference'] for v in row]+[c['capacity_minimum_now']]
    lines.append(' '.join(map(str,values)))
python_seconds = time.perf_counter()-start
(RUN/'cases.txt').write_text('\n'.join(lines)+'\n')
(RUN/'cases.json').write_text(json.dumps(cases,indent=2)+'\n')
(RUN/'expected.json').write_text(json.dumps(expected,indent=2)+'\n')
command = ['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-I',str(ROOT),str(RUN/'check.cpp'),'-o',str(RUN/'check')]
with (RUN/'build.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
start=time.perf_counter()
run=['conda','run','-n','kaggriculture',str(RUN/'check'),str(RUN/'cases.txt'),str(RUN/'actual.txt')]
subprocess.run(run,check=True)
cpp_process_seconds=time.perf_counter()-start
actual=[]
for line in (RUN/'actual.txt').read_text().splitlines():
    values=iter(map(int,line.split()));worst,total,count,n=[next(values) for _ in range(4)]
    plan=[[next(values),next(values)] for _ in range(n)]
    n=next(values);scores=[[next(values) for _ in range(4)] for _ in range(n)]
    assert list(values)==[]
    actual.append({'plan':plan,'worst_gain':worst,'sum_gain':total,'plans':count,'scores':scores})
assert len(actual)==len(expected)
differences=[{'case':i,'expected':a,'actual':b} for i,(a,b) in enumerate(zip(expected,actual)) if a!=b]
report={'completed_utc':datetime.now(timezone.utc).isoformat(),'cases':len(cases),'differences':differences,
    'python_oracle_seconds':python_seconds,'cpp_process_seconds_including_conda':cpp_process_seconds,
    'commands':[command,run],'scope':'Pure lot-search core with default game prices, <=100 own/rival units, <=8-turn horizon, minimum-current-sale and caller capacity constraints. This is not full-agent parity or a strength test.'}
(RUN/'CHECKS.json').write_text(json.dumps(report,indent=2)+'\n')
assert not differences, differences[:2]
sources=[SOURCE/'scheduler.py',SOURCE/'mechanics.py',SOURCE/'reference/decision/decision.py',RUN/'lots.hpp',RUN/'check.cpp',Path(__file__)]
(RUN/'LINEAGE.json').write_text(json.dumps({'source_commit':'7c50bbfb41027f31a2d4bc9470424e815f1fcef1',
    'archive_sha256':'7b58fa06da778b1519b81d509d28dff3481b3bbcc7a2d656e8bdfe4a22540524',
    'author':'Bryce Xavier Muhlnickel / TokenJunkieLabs; exact upstream attributions retained in NOTICE.',
    'translation':'MarketPath/optimize_lot C++ translation, root exact market prices; fixed arrays replace Python candidate sets and quote caches. Capacity/funding remain caller contracts. No Arlene route/controller copied into this primitive.',
    'sources':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},'validation':report['scope']},indent=2)+'\n')
for name in ['LICENSE','NOTICE']:(RUN/name).write_bytes((SOURCE/name).read_bytes())
print('All',len(cases),'lot plans, scenario receipts and scores match the pinned source.')
print('Oracle seconds',python_seconds,'C++ process incl conda',cpp_process_seconds)
