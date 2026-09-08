from pathlib import Path
import csv
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
course = RUN / 'wool_days_1005'
rows = list(csv.DictReader((course / 'compiled.csv').open()))
selected = [r for r in rows if r['solved'] == r['endpoint_equal'] == r['cash_equal'] == '1']
assert selected
code = '#pragma once\n#include "transfer.hpp"\nnamespace compositions::v52_family {\n'
code += 'inline const std::vector<GuardedDay>& improved_days(){static const auto value=[]{std::vector<GuardedDay> result;\n'
hashes = {}
for row in selected:
    day = int(row['day'])
    folder = course / 'proposals' / f'{course.name}_{day}'
    guard_path, actions_path = folder / 'guard.txt', folder / 'combined_schedule.txt'
    guard = [list(map(int, line.split())) for line in guard_path.read_text().splitlines()]
    actions = [list(map(int, line.split())) for line in actions_path.read_text().splitlines()]
    assert len(guard) == 103 and len(actions) == 24
    assert all(len(a) == 2 + 3 * (a[0] + a[1]) for a in actions)
    code += f'{{GuardedDay g;g.plan.day={day};g.quadrants={guard[0][1]};\n'
    code += 'g.tiles={{' + ','.join('{{' + ','.join(map(str, r[1:])) + '}}' for r in guard[1:101]) + '}};\n'
    for field, values in [('check', [r[0] for r in guard[1:101]]), ('shed', guard[101]), ('seeds', guard[102])]:
        code += 'g.' + field + '={' + ','.join(map(str, values)) + '};\n'
    code += 'constexpr int data[]={' + ',\n'.join(','.join(map(str, a)) for a in actions) + '};\n'
    code += '''int p=0;for(auto& a:g.plan.actions){a.n_units=data[p++];a.n_orders=data[p++];
        for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(data[p]),uint8_t(data[p+1]),data[p+2]};p+=3;}
        for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(data[p]),uint8_t(data[p+1]),data[p+2]};p+=3;}
        a.finalize();}result.push_back(std::move(g));}\n'''
    for path in [guard_path, actions_path, folder / 'problem.json']:
        hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
code += 'return result;}();return value;}\n'
code += '''class OptimizedTransfer:public Transfer {
    int selected_=-1;uint32_t matched_=0;
public:
    OptimizedTransfer(int threshold=2,int rule=0):Transfer(threshold,false,rule){(void)improved_days();}
    void reset(const kag::agent::AgentInit& init){Transfer::reset(init);selected_=-1;matched_=0;}
    uint32_t matched_days()const{return matched_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a){
        Transfer::act(o,budget,a);
        if(o.hour==0){selected_=-1;if(selected_route()==1)for(int i=0;i<int(improved_days().size());++i)
            if(improved_days()[i].plan.day==o.day && improved_days()[i].matches(o)){selected_=i;break;}}
        if(selected_<0)return;
        matched_|=uint32_t(1)<<o.day;a=improved_days()[selected_].plan.actions[o.hour];
        for(int u=a.n_units;u<o.self().n_units;++u)a.units[u]={};
        a.n_units=o.self().n_units;a.finalize();
    }
};
}\n'''
(RUN / 'source/improved.hpp').write_text(code)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
for threshold in [0, 2, 4]:
    name = f'v52_transfer_wool{threshold}_h18'
    package = RUN / 'proposals' / name
    assert name not in catalog and not package.exists()
    (package / 'source').mkdir(parents=True)
    (package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/improved.hpp"\n'
        f'namespace kag::agents::{name}{{class Agent:public compositions::v52_family::OptimizedTransfer{{public:Agent():OptimizedTransfer({threshold}){{}}'
        f'static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp', 'type': f'kag::agents::{name}::Agent',
                'sources': ['source/agent.cpp', *[os.path.relpath(EXP / f'league/{dep}/source/agent.cpp', package) for dep in ['top_replay_library', 'public_router']]]}
    (package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (package / 'IMPORT.json').write_text(json.dumps({'parent': f'v52_transfer_wool{threshold}', 'donor': 'public_router_v52',
        'source_lineage': 'league/public_router_v52/IMPORT.json', 'compiler': 'day_solver V30', 'source_course_seed': 1005,
        'source_opponent': 'late_value_s32_t0_r05', 'days': [int(r['day']) for r in selected],
        'conditional_hire_savings': sum(int(r['hire_saving']) for r in selected), 'source_sha256': hashes,
        'gates': 'Only active donor route1 and full physical starting day guard; complete donor fallback day on mismatch.',
        'status': 'Experimental. Individual fixed-rival source-day endpoint/cash checks pass; full-game combination pending.'}, indent=2) + '\n')
    (package / 'README.md').write_text(f'# {name}\n\nDay6 observed wool-family transfer with {len(selected)} locally rebuilt whole-day schedules. '
        'Only active donor route1 may use a physically matching day. Shared immutable calendar and independent per-instance selection. '
        'Experimental; exact source and compiler checks in IMPORT.json.\n')
    catalog[name] = str(package.relative_to(ROOT))
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
(RUN / 'DAY_LIBRARY.json').write_text(json.dumps({'days': selected, 'total_conditional_saving': sum(int(r['hire_saving']) for r in selected), 'source_sha256': hashes}, indent=2) + '\n')
print('Packaged', len(selected), 'days; conditional saving', sum(int(r['hire_saving']) for r in selected))
