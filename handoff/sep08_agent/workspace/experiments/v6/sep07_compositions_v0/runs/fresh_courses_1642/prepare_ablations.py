"""Format exact observed physical entry guards and component-control packages."""
import hashlib
import json
import sys
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from generate_public_routes import ITEMS


def key(tile, day):
    if tile is None:
        return [0, -1, -1] + [0] * 9
    if isinstance(tile, str):
        assert tile in ('LOCKED', 'WEED')
        return [1 if tile == 'LOCKED' else 2, -1, -1] + [0] * 9
    result = [{'WEED': 2, 'COOP': 3, 'PASTURE': 4, 'PLANT': 5}[tile['kind']], -1, -1] + [0] * 9
    if tile['kind'] == 'PLANT':
        result[1] = ITEMS.index(tile['crop'])
        result[3:6] = [day - tile['planted_day'], tile['yield_units'], tile['consecutive_unwatered']]
        result[7:9] = [max(0, tile['fertilized_until_day'] - day + 1), int(tile['watered_today'])]
    if tile.get('animal'):
        result[2] = ITEMS.index(tile['animal'])
        result[3:7] = [day - tile['placed_day'], tile['yield_units'], tile['consecutive_unfed'], tile['pending_care_bonus']]
        result[9:12] = [int(tile['fed_today']), int(tile['cared_today']), int(tile['fertilizer_available'])]
    return result


def main():
    programs = json.loads((RUN / 'library/IMPORT.json').read_text())['programs']
    source = programs[25]['donors'][0]
    path = EXP / f"replays/episode-{source['episode']}-replay.json"
    replay = json.loads(path.read_text())
    code = '#pragma once\n#include "../../include/guarded_day.hpp"\nnamespace compositions::fresh_bohann {\n'
    code += 'inline GuardedDay entry_guard(int day) { GuardedDay g{};\n'
    for day in (1, 6):
        o = replay['steps'][day * 24][source['seat']]['observation']
        f = o['farms'][source['seat']]
        assert f['farmer'] == [4, 4] and not f['hands']
        keys = [key(tile, day) for row in f['tiles'] for tile in row]
        code += f'if(day=={day}){{g.plan.day={day};g.quadrants={len(f["unlocked_quadrants"])};\n'
        code += 'g.tiles={{' + ','.join('{{' + ','.join(map(str, row)) + '}}' for row in keys) + '}};\n'
        code += 'g.check.fill(true);\n'
        for name, count in [('shed', 12), ('seeds', 5)]:
            data = [o['private'][name].get(item, 0) for item in ITEMS[:count]]
            code += f'g.{name}={{' + ','.join(map(str, data)) + '};\n'
        code += 'return g;}\n'
    code += 'if(day!=30)std::abort();return g;}\n}\n'
    (RUN / 'entry_guards.hpp').write_text(code)
    candidates = [
        ('fresh_bohann25_opening', True, 30), ('fresh_bohann25_suffix1', False, 1),
        ('fresh_bohann25_suffix6', False, 6), ('fresh_bohann25_opening_suffix6', True, 6)]
    catalog_path = EXP / 'configs/league.json'
    catalog = json.loads(catalog_path.read_text())
    metadata = []
    for name, opening, day in candidates:
        folder = RUN / 'proposals' / name
        assert not folder.exists() and name not in catalog
        (folder / 'source').mkdir(parents=True)
        (folder / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../ablation_policy.hpp"
namespace compositions::{name} {{
class Agent: public fresh_bohann::Policy {{public:
    Agent():Policy({str(opening).lower()},{day}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
        (folder / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../library/source/agent.cpp',
            '../../../../league/top_replay_library/source/agent.cpp', '../../../../league/public_router/source/agent.cpp']}, indent=2) + '\n')
        (folder / 'README.md').write_text(f'# {name}\n\nComponent ablation on crop_mix_t2_wheat. Opening market replacement: {opening}; whole-course entry day: {day} (30 disables it). Entry requires the exact observed public-replay physical state. A selected continuation receives the existing terminal recall/liquidation. Full-game tests determine funding, production and opponent consequences; no strength claim from the entry guard. Source: Bohann Wang, episode {source["episode"]}, seat {source["seat"]}, submission {source["submission_id"]}.\n')
        catalog[name] = str(folder.relative_to(ROOT))
        metadata.append({'name': name, 'opening': opening, 'entry_day': day})
    catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
    (RUN / 'ABLATION_LINEAGE.json').write_text(json.dumps({'donor': source, 'components': metadata,
        'guard_scope': 'All 100 tile keys, shed, seeds, quadrants, empty farmer inventory and dawn unit position; no relaxed cells.',
        'hypothesis': 'The fresh King specialist may owe its advantage to opening trades, later composition or their interaction. Test independently.',
        'guard_source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'local_reuse': ['Current crop_mix_t2_wheat base', 'GuardedDay physical matching', 'terminal_layer mode3, with Dusta/King recall and Lynn liquidation lineage'],
        'selection_scope': 'Experimental unconditional component controls; no future-shop or opponent-private input.'}, indent=2) + '\n')
    print('Prepared', len(candidates), 'component controls with exact source guards')


if __name__ == '__main__':
    main()
