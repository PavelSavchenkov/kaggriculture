from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
model = json.loads((RUN / 'MODEL_CHECKS.json').read_text())
assert model['daytime_solo_steps_exact'] == 2760
for path, expected in model['source_hashes'].items():
    assert hashlib.sha256((EXP / path).read_bytes()).hexdigest() == expected
SOURCE = EXP / 'runs/joint_day_witnesses_sep08_001'
results = json.loads((SOURCE / 'RESULTS.json').read_text())
keys = ['crop', 'animal', 'age_days', 'stored_units', 'consecutive_dry_days', 'pending_care_bonus',
        'fertilizer_days_remaining', 'watered_today', 'fed_today', 'cared_today', 'fertilizer_available']
kinds = {'empty': 0, 'locked': 1, 'weed': 2, 'coop': 3, 'pasture': 4, 'crop': 5}
source_paths = []
code = '#pragma once\n#include "program.hpp"\nnamespace compositions::day_program_data {\n'
functions = {}
for case in ['p355', 'p362']:
    functions[case] = []
    for day in [14, 15, 16]:
        for mode in [0, 1]:
            agent = f'joint_routes_{case}_m{mode}'
            row = next(r for r in results['cases'] if r['agent'] == agent and r['day'] == day and r['augment'] == 1 and r['remove_hires'] == 2)
            assert row['solved'] and row['full_endpoint_equal'] and row['cash_equal']
            folder = SOURCE / f'compiled/{agent}/day{day}_augment1_remove2'
            problem_path, schedule_path = folder / 'problem.json', folder / 'combined_schedule.txt'
            source_paths += [problem_path, schedule_path]
            problem = json.loads(problem_path.read_text())
            work = {w['tile'] for w in problem['tile_work']}
            owned = sum(t['state']['kind'] != 'locked' for t in problem['start']['managed_tiles'])
            assert owned % 25 == 0
            name = f'{case}_d{day}_source{mode}'
            functions[case].append(name)
            code += f'inline day_programs::Program {name}() {{\nday_programs::Program result{{}};auto& p=result.start;p.plan.day={day};p.quadrants={owned//25};\n'
            code += 'p.shed={' + ','.join(map(str, problem['start']['shed'])) + '};\n'
            code += 'p.seeds={' + ','.join(map(str, problem['start']['seeds'])) + '};\n'
            for tile in problem['start']['managed_tiles']:
                cell = tile['y']*10+tile['x']
                state = tile['state']
                values = [kinds[state['kind']], *[int(state[key]) for key in keys]]
                code += f'p.tiles[{cell}]={{' + ','.join(map(str, values)) + '};\n'
                if state['kind'] in ['crop', 'pasture', 'coop'] or cell in work:
                    code += f'p.check[{cell}]=true;\n'
            for end in problem['end_tiles']:
                state = end['state']
                values = [kinds[state['kind']], *[int(state[key]) for key in keys]]
                code += f'result.end[{end["tile"]}]={{' + ','.join(map(str, values)) + '};\n'
            values = [int(v) for line in schedule_path.read_text().splitlines() for v in line.split()]
            code += 'constexpr int data[]={' + ','.join(map(str, values)) + '};\n'
            code += '''const int* data_ptr=data;
for(auto& a:p.plan.actions){a.n_units=*data_ptr++;a.n_orders=*data_ptr++;
for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(data_ptr[0]),uint8_t(data_ptr[1]),data_ptr[2]};data_ptr+=3;}
for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(data_ptr[0]),uint8_t(data_ptr[1]),data_ptr[2]};data_ptr+=3;}
a.finalize();}return result;}
'''
code += '}\n'
(RUN / 'days.hpp').write_text(code)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['p355', 'p362']:
    for mode in range(4):
        name = f'day_program_{case}_m{mode}'
        path = RUN / 'proposals' / name
        (path / 'source').mkdir(parents=True, exist_ok=False)
        calls = ','.join(f'day_program_data::{f}()' for f in functions[case]) if mode else ''
        (path / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../days.hpp"
#include "../../../../joint_day_routes_sep08_001/proposals/joint_routes_{case}_m0/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public day_programs::Agent<joint_routes_{case}_m0::Agent,{mode}> {{
public:
    Agent():day_programs::Agent<joint_routes_{case}_m0::Agent,{mode}>(std::vector<day_programs::Program>{{{calls}}}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']}, indent=2) + '\n')
        (path / 'README.md').write_text(f'# {name}\n\nComplete cold-farm controller with checked day14..16 programs, mode{mode}. Mode0 original control; mode1 exact source tile/stock guard; mode2 exact tile state with current-stock forward replay; mode3 tile kind/species guard plus replay and redundant-action skipping. Models assume rival PASS and no random weeds, using only observations and public config. Actual unit counts and required physical actions are checked each turn; abandon a program to the underlying compiler if they fail. Market changes can still alter financing and full-game strength. All policy/model/counter state is per-instance. See ../../LINEAGE.json. Experimental, not promoted.\n')
        assert name not in catalog
        catalog[name] = str(path.relative_to(ROOT))
        names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
source_paths += [RUN / 'model.hpp', RUN / 'program.hpp', RUN / 'days.hpp', RUN / 'prepare.py']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(), 'variants': names,
    'source': 'Twelve locally solved augmented/two-fewer-worker day programs from joint_day_witnesses_sep08_001, four exact source games. No donor worker routes newly copied.',
    'hypothesis': 'Exact stock equality discards reusable schedules. Validate a day program against the current own observation instead, then separately relax biological quantities with redundant-action handling while preserving layout/species and end shapes.',
    'model_evidence': model,
    'selection': 'Deterministic order: same-day original-controller program before joint-controller program. No selection uses a seed, opponent identifier or future shop.',
    'limits': ['Only days14..16 and two cold farms; not a general season compiler.', 'Rival PASS forecast is an approximation; actual financing may differ and force fallback.', 'A physically valid program is not guaranteed to improve economic value; full-game comparisons are required.'],
    'discovery': {'seed_start': 1000, 'seeds': 8, 'seat_mode': 'both', 'opponents': ['public_router', 'observed_sale_lead_start_216', 'matching_original_controller', 'pass']},
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths}}, indent=2) + '\n')
print('Prepared eight complete C++ day-program/control policies.')
