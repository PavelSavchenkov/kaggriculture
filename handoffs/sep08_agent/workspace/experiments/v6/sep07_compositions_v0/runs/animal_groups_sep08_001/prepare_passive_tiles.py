"""Carry actual untouched empty/weed tiles through a compiled day."""
from datetime import datetime, timezone
from pathlib import Path
from copy import deepcopy
import hashlib
import json

RUN = Path(__file__).resolve().parent
out = RUN / 'passive_terminal_contracts'
out.mkdir(exist_ok=False)
cases = []
for name, seed, parent in [('sheep_3',1016,'terminal_groups_30s'),('sheep_12',1016,'terminal_groups_30s'),
    ('sheep_13',1016,'terminal_groups_30s'),('sheep_triple',1016,'terminal_groups_30s'),
    ('cow_9',1014,'market_timing_30s'),('cow_triple',1014,'market_timing_30s')]:
    prefix = RUN / parent / name
    for extra in (0,1,2):
        source = prefix / 'days/29' / f'problem_h{extra}.json'
        problem = json.loads(source.read_text())
        working = {w['tile'] for w in problem['tile_work'] if w['actions']}
        before = deepcopy(problem)
        corrections = []
        for target in problem['end_tiles']:
            cell = target['tile']
            start = problem['start']['managed_tiles'][cell]
            assert start['x'] + 10*start['y'] == cell
            if cell not in working and start['state']['kind'] in ['empty','weed'] and target['state']['kind'] in ['empty','weed']:
                if target['state'] != start['state']:
                    corrections.append({'cell':cell,'before':target['state'],'after':start['state']})
                    target['state'] = deepcopy(start['state'])
        assert corrections
        path = out / f'{name}_h{extra}.json'
        path.write_text(json.dumps(problem,indent=2)+'\n')
        cases.append({'name':f'{name}_h{extra}','agent_case':name,'seed':seed,'extra_hires':extra,
            'prefix':str(prefix.relative_to(RUN)),'problem':str(path.relative_to(RUN)),
            'orders':str((prefix/'days/29'/f'orders_h{extra}.txt').relative_to(RUN)),
            'source_problem':str(source.relative_to(RUN)),
            'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'corrections':corrections})
(out/'PROTOCOL.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'scope':'No-work empty/weed tiles must preserve actual start state. The old contract copied another trajectory\'s stochastic weeds. No active tile, work, purchase, sale or inventory requirement changes.',
    'cases':cases},indent=2)+'\n')
old = RUN/'source/compile_v6.cpp'
new = RUN/'source/compile_v7.cpp'
assert not new.exists()
source = old.read_text()
point = '        auto markets=source.own;std::array<int,N_ITEMS> flow_delta{};'
assert source.count(point) == 1
source = source.replace(point, '''        // Unworked empty/weed squares belong to the actual trajectory.
        // A compiler cannot require a random weed to appear or disappear.
        for(int cell=0;cell<100;++cell){
            const auto& tile=sim.st.farms[0].tiles[cell/10][cell%10];
            auto& end=*problem.required_end_tiles[cell].exact_state;
            if(source_work(problem,cell).empty() && (tile.kind==T_EMPTY || tile.kind==T_WEED) &&
                (end.kind==ManagedTileKind::EMPTY || end.kind==ManagedTileKind::WEED))
                end=managed(tile,day+1);
        }
''' + point)
new.write_text(source)
cmake=RUN/'CMakeLists.txt';text=cmake.read_text()
assert text.count('hinted_solve_v2 final_audit)') == 1
cmake.write_text(text.replace('hinted_solve_v2 final_audit)', 'hinted_solve_v2 final_audit compile_v7)'))
print('Prepared18 corrected final contracts; compiler_v7 applies the same actual-state rule.')
