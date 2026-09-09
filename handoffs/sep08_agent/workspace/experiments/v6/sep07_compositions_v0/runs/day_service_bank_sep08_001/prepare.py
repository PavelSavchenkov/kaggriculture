from pathlib import Path
import hashlib,json
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
SOURCE=EXP/'runs/day_service_contract_sep08_001'
report=json.loads((SOURCE/'RESULTS.json').read_text())
assert report['source_full_game_equal']
keys=['crop','animal','age_days','stored_units','consecutive_dry_days','pending_care_bonus','fertilizer_days_remaining','watered_today','fed_today','cared_today','fertilizer_available']
kinds={'empty':0,'locked':1,'weed':2,'coop':3,'pasture':4,'crop':5}
code='#pragma once\n#include "../day_programs_sep08_001/program.hpp"\nnamespace compositions::service_bank_data {\n'
paths=[]
for remove in [0,2]:
 row=next(r for r in report['cases'] if r['augment']==1 and r['remove_hires']==remove)
 assert row['solved'] and row['full_endpoint_equal'] and row['cash_equal']
 folder=SOURCE/f'compiled/day_program_p362_m3/day16_augment1_remove{remove}'
 paths += [folder/'problem.json',folder/'combined_schedule.txt']
 p=json.loads((folder/'problem.json').read_text());work={w['tile'] for w in p['tile_work']}
 owned=sum(t['state']['kind']!='locked' for t in p['start']['managed_tiles'])
 code+=f'inline day_programs::Program day16_r{remove}() {{day_programs::Program result{{}};auto& p=result.start;p.plan.day=16;p.quadrants={owned//25};\n'
 code+='p.shed={'+','.join(map(str,p['start']['shed']))+'};\np.seeds={'+','.join(map(str,p['start']['seeds']))+'};\n'
 for tile in p['start']['managed_tiles']:
  cell=tile['y']*10+tile['x'];state=tile['state'];v=[kinds[state['kind']],*[int(state[k]) for k in keys]]
  code+=f'p.tiles[{cell}]={{'+','.join(map(str,v))+'};\n'
  if state['kind'] in ['crop','pasture','coop'] or cell in work:code+=f'p.check[{cell}]=true;\n'
 for tile in p['end_tiles']:
  state=tile['state'];v=[kinds[state['kind']],*[int(state[k]) for k in keys]]
  code+=f'result.end[{tile["tile"]}]={{'+','.join(map(str,v))+'};\n'
 data=[int(v) for line in (folder/'combined_schedule.txt').read_text().splitlines() for v in line.split()]
 code+='constexpr int data[]={'+','.join(map(str,data))+'};\n'
 code+='const int* q=data;for(auto& a:p.plan.actions){a.n_units=*q++;a.n_orders=*q++;for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(q[0]),uint8_t(q[1]),q[2]};q+=3;}for(int j=0;j<a.n_orders;++j){a.orders[j]={uint8_t(q[0]),uint8_t(q[1]),q[2]};q+=3;}a.finalize();}return result;}\n'
code+='}\n';(RUN/'days.hpp').write_text(code)
catalog=json.loads((EXP/'configs/league.json').read_text());names=[]
for variant,mode,remove in [(0,0,2),(1,2,2),(2,3,2),(3,3,0)]:
 name=f'service_bank_p362_m{variant}';names.append(name);p=RUN/'proposals'/name;(p/'source').mkdir(parents=True)
 call='' if variant==0 else f'service_bank_data::day16_r{remove}()'
 (p/'source/agent.hpp').write_text(f'#pragma once\n#include "../../../days.hpp"\n#include "../../../../day_programs_sep08_001/proposals/day_program_p362_m3/source/agent.hpp"\nnamespace compositions::{name} {{class Agent:public day_programs::Agent<day_program_p362_m3::Agent,{mode}> {{public: Agent():day_programs::Agent<day_program_p362_m3::Agent,{mode}>(std::vector<day_programs::Program>{{{call}}}){{}} static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
 (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
 (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
 (p/'README.md').write_text(f'# {name}\n\nAdds the actual next-day16 contract after existing p362 day14/15 programs. Variant{variant}: mode{mode}, remove{remove} hires. Variant0 is unchanged old program control; variant1 exact biological state with current-stock replay; variant2 relaxed shape/species with two fewer workers; variant3 relaxed with original workers. Uses only observed state. See ../../LINEAGE.json.\n')
 assert name not in catalog;catalog[name]=str(p.relative_to(ROOT))
(EXP/'configs/league.json').write_text(json.dumps(catalog,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'variants':names,'parent':'day_program_p362_m3','source':'runs/day_service_contract_sep08_001','source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},'hypothesis':'Compile missing current-state service into a reusable next-day program; preserve previous improved days.','limits':['Only one new actual day16 state; broader compiler strength is unproven.','Relaxed entry can alter subsequent service needs.','Physical forecast assumes rivalPASS and no random weeds.']},indent=2)+'\n')
print('Prepared four actual-state service-bank policies.')
