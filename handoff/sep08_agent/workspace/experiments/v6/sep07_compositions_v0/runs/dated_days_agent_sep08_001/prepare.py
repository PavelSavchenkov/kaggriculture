"""Compile checked day schedules into guarded complete cold-farm policies."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
first = EXP/'runs/dated_day_service_sep08_001'
second = EXP/'runs/dated_day_service_sep08_002'
keys = ['crop','animal','age_days','stored_units','consecutive_dry_days','pending_care_bonus',
        'fertilizer_days_remaining','watered_today','fed_today','cared_today','fertilizer_available']
kinds = {'empty':0,'locked':1,'weed':2,'coop':3,'pasture':4,'crop':5}
sources = []
code = '#pragma once\n#include "../../include/guarded_day.hpp"\nnamespace compositions::dated_days_data {\n'
for prefix,base,remove,days in [('labor',first,2,[14,18,22,26]),('feed',second,0,[18,26]),('combined',second,2,[14,18,22,26])]:
    for day in days:
        folder = base/f'compiled/day{day}_remove{remove}'
        report = json.loads((base/'RESULTS.json').read_text())
        r = next(r for r in report['cases'] if r['day']==day and r['remove_hires']==remove)
        assert r['solved'] and r['full_endpoint_equal'] and r['cash_equal']
        problem_path = folder/'problem.json';schedule_path = folder/'combined_schedule.txt'
        sources += [problem_path,schedule_path]
        p = json.loads(problem_path.read_text())
        work = {w['tile'] for w in p['tile_work']}
        name = f'{prefix}_{day}'
        values = [int(v) for line in schedule_path.read_text().splitlines() for v in line.split()]
        code += f'inline GuardedDay {name}() {{\nGuardedDay result{{}};result.plan.day={day};result.quadrants=3;\n'
        code += 'result.shed={'+','.join(map(str,p['start']['shed']))+'};\n'
        code += 'result.seeds={'+','.join(map(str,p['start']['seeds']))+'};\n'
        for tile in p['start']['managed_tiles']:
            cell=10*tile['y']+tile['x'];s=tile['state']
            key=[kinds[s['kind']]]+[int(s[k]) for k in keys]
            code += f'result.tiles[{cell}]={{'+','.join(map(str,key))+'};\n'
            if s['kind'] in ['crop','pasture','coop'] or cell in work:
                code += f'result.check[{cell}]=true;\n'
        code += 'constexpr int data[]={'+','.join(map(str,values))+'};\n'
        code += '''const int* p=data;
for(auto& a:result.plan.actions){a.n_units=*p++;a.n_orders=*p++;
for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
a.finalize();}
return result;}
'''
code += '}\n';(RUN/'days.hpp').write_text(code)
catalog_path=EXP/'configs/league.json';catalog=json.loads(catalog_path.read_text());names=[]
for mode,days in [('control',[]),('labor',[14,18,22,26]),('feed',[18,26]),('combined',[14,18,22,26])]:
    name=f'dated_days_{mode}';folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
    calls=','.join(f'dated_days_data::{mode}_{day}()' for day in days)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../days.hpp"
#include "../../../../dated_expansion_sep08_001/proposals/dated_expansion_p362/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public GuardedDayAgent<dated_expansion_p362::Agent> {{
public:
    Agent():GuardedDayAgent<dated_expansion_p362::Agent>(std::vector<GuardedDay>{{{calls}}}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../../compiler_care_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
    (folder/'README.md').write_text(f'# {name}\n\nComplete p362 cold-farm policy with {mode} guarded day schedules. Exact own tile, stock, seed, quadrant and morning worker state must match the compiled starting contract. Other days use the observation-driven composition compiler. No source game seed, rival-private stock or future shops enter the policy. Day checks are proven on their source case; wider full-season financing and activation are measured separately. Lineage in ../../LINEAGE.json. Not promoted.\n')
    assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
spec={'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,'baseline':'dated_expansion_p362',
    'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216','dated_expansion_p362','pass']},
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources+[RUN/'days.hpp',RUN/'prepare.py',EXP/'include/guarded_day.hpp']},
    'lineage':['Local larger farm p362 from dated_expansion_sep08_001.','Persistent day_solver solves this local farm actual tasks; no donor route copied.',
               'Existing GuardedDayAgent matches physical entry state and uses independently owned episode state.'],
    'limits':['Templates come from one exposed source game; activation across other shops/biological states may be sparse.',
              'Day endpoint and funding validation does not prove full-season strength; altered service can prevent later template matching.',
              'Do not substitute per-seed templates for general day-task generation and runtime input allocation.']}
(RUN/'LINEAGE.json').write_text(json.dumps(spec,indent=2)+'\n')
s=(EXP/'runs/dated_expansion_sep08_001/run_discovery.py').read_text().replace("'--games','4'","'--games','8'")
(RUN/'run_discovery.py').write_text(s)
print('Prepared',len(names),'complete guarded day policies.')
