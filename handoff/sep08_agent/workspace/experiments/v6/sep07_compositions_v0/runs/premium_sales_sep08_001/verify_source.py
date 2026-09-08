"""Compare typed order transforms with the inspected original V24 wrapper."""
from pathlib import Path
import ast
import copy
import hashlib
import json
import random
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
source=EXP/json.loads((RUN/'IMPORT.json').read_text())['reference']
tree=ast.parse(source.read_text())
assert isinstance(tree.body[-1],ast.FunctionDef) and tree.body[-1].name=='agent'
scope={'_step_of':lambda o:o['step'],'_int':int}
exec(compile(ast.Module(body=[tree.body[-1]],type_ignores=[]),str(source),'exec'),scope)
items=['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON','EGG','MILK','WOOL','FERTILIZER','GOOSE','COW','SHEEP']
# enum values are pinned to the persistent engine, checked by C++ below.
ops=['NONE','HIRE','BUY_LAND','BUY_SEED','BUY_PRODUCT','BUY_ANIMAL','SELL']
rng=random.Random(144216)
cases=[];expected=[]
for i in range(4096):
    step=[0,143,144,145,215,216,717,718][i%8]
    orders=[[rng.randrange(len(ops)),rng.randrange(len(items)),rng.choice([0,1,2,9,100])] for _ in range(i%11)]
    original={'farmer':['PASS'],'hands':[],'market':[[ops[op],items[item],n] for op,item,n in orders]}
    scope['_IMPL']=lambda observation,configuration=None:copy.deepcopy(original)
    result=scope['agent']({'step':step})
    expected.append([[ops.index(op),items.index(item),n] for op,item,n in result['market']])
    cases.append((step,orders))
build=RUN/'source_check';build.mkdir(exist_ok=False)
command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O2','-I',str(ROOT),str(RUN/'source/check_transform.cpp'),'-o',str(build/'check')]
with (build/'build.log').open('x') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
payload=str(len(cases))+'\n'+''.join(f'{step} {len(orders)} '+ ' '.join(str(v) for order in orders for v in order)+'\n' for step,orders in cases)
run=['conda','run','--no-capture-output','-n','kaggriculture',str(build/'check')]
result=subprocess.run(run,input=payload,text=True,capture_output=True,check=True)
lines=[line for line in result.stdout.splitlines() if line.strip()];assert len(lines)==len(cases)
for i,line in enumerate(lines):
    v=[int(x) for x in line.split()];assert len(v)==1+3*v[0]
    actual=[v[j:j+3] for j in range(1,len(v),3)]
    assert actual==expected[i],(i,cases[i],actual,expected[i])
(RUN/'SOURCE_PARITY.json').write_text(json.dumps({'cases':len(cases),'steps':[0,143,144,145,215,216,717,718],
    'orders_per_case':'0..10','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'build_command':command,'run_command':run,
    'scope':'Original inspected wrapper only; no notebook cells executed. Integer typed orders, full stable order equality. Other36ASTnodes equal prior verifiedV23.'},indent=2)+'\n')
print('V24 source transform parity4096cases.')
