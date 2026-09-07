"""Create immutable replay courses and own-flow models; no gameplay in Python."""
import csv
import hashlib
import json
import sys
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
sys.path.insert(0, str(EXP / 'scripts'))
from generate_public_routes import ITEMS, triple
from verify_public_router import pack

DONORS = [('cow', 156, 106371597, 1), ('sheep', 160, 106380079, 1), ('goose', 157, 106374592, 1)]
NAMES = ['atakan_cow', 'atakan_sheep', 'atakan_goose', 'atakan_demand', 'atakan_quotes', 'atakan_value_own', 'atakan_value_margin']


def main():
    source = RUN / 'source'
    source.mkdir(parents=True, exist_ok=True)
    (RUN / 'tests').mkdir(exist_ok=True)
    values, offsets, models, metadata = [], [], [], []
    reference = EXP / 'research/animal_decision_replays/atakan_three_way'
    for branch, (label, program, episode, seat) in enumerate(DONORS):
        directory = reference / label
        actions = json.loads((directory / 'raw_actions_719.json').read_text())
        replay_path = EXP / f'replays/episode-{episode}-replay.json'
        replay = json.loads(replay_path.read_text())
        row = []
        with (RUN / f'tests/source_{branch}.txt').open('w') as fixture:
            for step, action in enumerate(actions):
                units = [triple(x) for x in [action['farmer'], *action['hands']]]
                orders = [triple(x, True) for x in action['market']]
                row.append(len(values))
                values += [len(units), len(orders), *(v for t in units + orders for v in t)]
                obs = dict(replay['steps'][step][seat]['observation'], step=step)
                fixture.write(pack(obs, action, step == 0, full_tiles=True))
        offsets.append(row)
        daily = json.loads((directory / 'daily_service_harvest_trade.json').read_text())
        sales = [[0] * 9 for _ in range(30)]
        buys = [[0] * 9 for _ in range(30)]
        fixed = [0.] * 30
        harvest = [[0] * 9 for _ in range(30)]
        for day in daily:
            d = day['day']
            if d >= 10:
                for item, count in day['realized_harvest_units'].items():
                    if item in ITEMS[:9]:
                        harvest[d][ITEMS.index(item)] += count
            for trade in day['trades']:
                if 24*d + int(trade['hour']) < 226:
                    continue
                operation, item = trade['operation'], trade['item']
                if operation == 'SELL':
                    sales[d][ITEMS.index(item)] += int(trade['actual'])
                elif operation == 'BUY_PRODUCT':
                    buys[d][ITEMS.index(item)] += int(trade['actual'])
                else:
                    fixed[d] += float(trade['value'])
        # Partial day9 harvest needs exact event times rather than a whole-day total.
        for event in csv.DictReader((directory / 'unit_events.csv').open()):
            if event['operation'] == 'HARVEST' and int(event['day']) == 9 and int(event['result_index']) - 1 >= 226:
                item = {'COW': 'MILK', 'SHEEP': 'WOOL', 'GOOSE': 'EGG'}.get(event['source'], event['source'])
                harvest[9][ITEMS.index(item)] += int(event['quantity'])
        model = {'branch': branch, 'source_program': program, 'sales': sales, 'buys': buys, 'fixed_costs': fixed, 'harvest': harvest}
        models.append(model)
        metadata.append({'branch': branch, 'name': label, 'program': program, 'episode': episode, 'seat': seat,
                         'replay_sha256': hashlib.sha256(replay_path.read_bytes()).hexdigest(),
                         'source_import': str((directory / 'IMPORT.json').relative_to(EXP)),
                         'model_inputs_sha256': {f: hashlib.sha256((directory / f).read_bytes()).hexdigest() for f in ['daily_service_harvest_trade.json', 'unit_events.csv', 'raw_actions_719.json']}})
    code = '// Generated immutable source courses and intended own-flow templates.\n'
    code += 'inline constexpr int offsets[3][719]={\n' + ',\n'.join('{' + ','.join(map(str, x)) + '}' for x in offsets) + '\n};\n'
    code += 'inline constexpr int values[]={\n' + ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + '\n};\n'
    for key, name in [('sales', 'planned_sales'), ('buys', 'planned_buys'), ('harvest', 'planned_harvest')]:
        code += f'inline constexpr int {name}[3][30][9]=' + json.dumps([x[key] for x in models], separators=(',', ':')).replace('[', '{').replace(']', '}') + ';\n'
    code += 'inline constexpr double planned_fixed[3][30]=' + json.dumps([x['fixed_costs'] for x in models], separators=(',', ':')).replace('[', '{').replace(']', '}') + ';\n'
    (source / 'data.inc').write_text(code)
    (RUN / 'flow_models.json').write_text(json.dumps(models, indent=2) + '\n')
    formulas = ['Routing disabled: source156', 'Routing disabled: source160', 'Routing disabled: source157',
                'At226: sheep if any revealed Yarn; otherwise cow if milk-demand>=2; otherwise goose',
                'At226: sheep if current wool quote>=195; otherwise cow if milk quote>=180; otherwise goose',
                'At226 maximize projected own remaining whole-plan profit, expected unknown-shop demand weight0.35',
                'At226 maximize projected own profit minus projected public-rival-herd revenue, unknown-shop weight0.35']
    common = {'donors': metadata, 'shared_exact_normalized_action_prefix': 226,
              'original_branch_logic': 'Not supplied; all routing formulas below are locally inferred experiments',
              'flow_model': 'Recorded successful own product purchases/sales after226 are immutable quantity forecasts; fixed animal/seed/land/hire costs retained. No donor future quotes, shops or rival flows are runtime inputs.',
              'model_lineage': 'Whole-farm flow pricing follows local include/animal_investment_value.hpp; public-herd full-service forecast follows include/herd_forecast.hpp, which attributes the portfolio idea to Dmitrii Gluzdov S38. This implementation is independently stored here.',
              'limitations': ['Donor successful quantities assume donor logistics/funding; actual continuation can deviate', 'Known shops plus discounted expected unknown shops are a forecast, not future observations', 'Rival current animals only, full daily care/feed and sale; future crops, expansion, labor and private stocks are unknown', 'Daily midpoint pricing ignores intraday scheduling and cash feasibility', 'Goose donor before226 has20wheat versus28 in cow/sheep; identical source actions do not imply identical realized states'],
              'local_formula_source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [EXP / 'include/animal_investment_value.hpp', EXP / 'include/herd_forecast.hpp']}}
    for mode, name in enumerate(NAMES):
        target = RUN / 'proposals' / name
        (target / 'source').mkdir(parents=True, exist_ok=True)
        (target / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/agent.hpp"\n' +
            f'namespace compositions::{name} {{class Agent:public atakan_portfolio::Agent {{public:Agent():atakan_portfolio::Agent({mode}){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}} }};}}\n')
        (target / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (target / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../source/agent.cpp']}, indent=2) + '\n')
        (target / 'IMPORT.json').write_text(json.dumps({**common, 'mode': mode, 'formula': formulas[mode]}, indent=2) + '\n')
    (RUN / 'LINEAGE.json').write_text(json.dumps(common, indent=2) + '\n')
    print('Generated3source courses and7policy candidates')


if __name__ == '__main__':
    main()
