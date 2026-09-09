"""Port V25 using the already verified, unchanged V23 execution methods."""
import ast
import hashlib
import json
import shutil
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
upstream = EXP / 'research/refresh_sep08_1508/notebooks/ahmed/extracted/main.py'
previous = EXP / 'research/refresh_2302/notebook_audit/ahmed/reference_v23.py'
def definitions(path):
    return {node.name: ast.dump(node) for node in ast.parse(path.read_text()).body
            if isinstance(node, (ast.FunctionDef, ast.ClassDef))}
old, new = definitions(previous), definitions(upstream)
same = [name for name in old if name in new and old[name] == new[name]]
assert all(name in same for name in ('Chassis', '_View', '_is_noop', '_shed_adjacent', 'make_agent'))
folder = RUN / 'proposals/ahmed_v25'
(folder / 'source').mkdir(parents=True, exist_ok=False)
parent = EXP / 'league/ahmed_v23'
header = (parent / 'source/agent.hpp').read_text().replace('kag::agents::ahmed_v23', 'compositions::ahmed_v25').replace('"ahmed_v23"', '"ahmed_v25"')
(folder / 'source/agent.hpp').write_text(header)
source = (parent / 'source/agent.cpp').read_text().replace('namespace kag::agents::ahmed_v23', 'namespace compositions::ahmed_v25')
source = source.replace('namespace compositions::ahmed_v25 {', 'namespace compositions::ahmed_v25 {\nusing namespace kag;')
begin = source.index('int remaining(')
source = source[:begin] + source[source.index('void tape(', begin):]
begin = source.index('std::array<int,9> ordered_prices(')
source = source[:begin] + '}\n' + source[source.index('void Agent::clear_state()', begin):]
begin = source.index('    if(!first_')
end = source.index('    tape(route_', begin)
source = source[:begin] + '''    if(!production_ && o.step>=144){
        route_=0;for(int i=0;i<o.n_shops;++i)if(o.shops[i]==SHOP_YARN_STORE)route_=1;
        production_=true;
    }
    if(!crop_ && o.step>=648){route_=o.market.inventory[EGG]<=9888?2:3;crop_=true;}
''' + source[end:]
begin = source.index('    budget_guard(')
end = source.index('    for(int i=0;i<a.n_orders;++i){auto& m=a.orders[i];if(m.op==M_NONE', begin)
source = source[:begin] + '''    if(o.step>=718){
        for(int u=0;u<a.n_units;++u){
            bool cargo=false;for(int i=0;i<N_ITEMS;++i)cargo|=o.own.inv[u][i]>0;
            a.units[u]={uint8_t(adjacent(o,u) && cargo?OP_DROP:OP_PASS),0,1};
        }
        a.n_orders=0;const auto terminal=projected(o,a);
        for(int i=0;i<9;++i)if(terminal[i]>0)a.orders[a.n_orders++]={M_SELL,uint8_t(i),terminal[i]};
        std::stable_sort(a.orders,a.orders+a.n_orders,[&](const auto& x,const auto& y){
            return int64_t(o.market.prices[x.item])*x.n>int64_t(o.market.prices[y.item])*y.n;
        });
    }
''' + source[end:]
(folder / 'source/agent.cpp').write_text(source)
shutil.copyfile(EXP / 'runs/yusuke_port_sep08_001/source/data.inc', folder / 'source/data.inc')
shutil.copyfile(parent / 'LICENSE.txt', folder / 'LICENSE.txt')
(folder / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'ahmed_v25',
    'header': 'source/agent.hpp', 'type': 'compositions::ahmed_v25::Agent', 'sources': ['source/agent.cpp']}, indent=2) + '\n')
(folder / 'README.md').write_text('''# Ahmed V25 C++ port

Four Yusuke Shop Router0908 production tapes, observed-Yarn day6 branch and
public egg-inventory day27 branch. Ahmed V25 enables weed repair and sale leading
and adds final-step adjacent-worker deposits with value-sorted liquidation.

Exact source and component lineage are in ../../LINEAGE.json. Apache2.0 chassis
credits thomastschinkel, yhay81 and tetsutani; original tape replay episodes and
artifact rating are unspecified. This port has no optimization claim. Source
parity, operational tests and competitive evaluation are pending. All mutable
route, repair queues and sale suppression belong to each resettable instance.
''')
(RUN / 'LINEAGE.json').write_text(json.dumps({'upstream': 'https://www.kaggle.com/code/ahmedberatozer/notebook07b5f4563e',
    'upstream_sha256': hashlib.sha256(upstream.read_bytes()).hexdigest(), 'version': 'V25 EXP149',
    'unchanged_python_definitions_from_verified_v23': same,
    'cpp_execution_parent': str(parent.relative_to(EXP)),
    'tapes': 'research/refresh_sep08_1108/notebook_audit/yusuke/actions.json.txt',
    'tapes_equal_v25': True,
    'changes': ['Four Yusuke routes and original thresholds replace V23 production routes.',
        'Enable only alignment, weed repair and sale leading; remove disabled economic layers.',
        'Final step718 deposits adjacent carried cargo and liquidates by current lot value.'],
    'rating': None, 'original_replay_provenance': 'Unspecified by source',
    'source_parity': 'Pending', 'scope': 'Faithful public C++ agent port, not an inhouse improvement.'}, indent=2) + '\n')
registry_path = EXP / 'configs/league.json'
registry = json.loads(registry_path.read_text());assert 'ahmed_v25' not in registry
registry['ahmed_v25'] = str(folder.relative_to(ROOT))
registry_path.write_text(json.dumps(registry, indent=2) + '\n')
print('Prepared AhmedV25 C++ package. Source parity pending.')
