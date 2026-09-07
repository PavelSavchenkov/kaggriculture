"""Format frozen public data only; agent behavior runs in C++."""
import hashlib
import json
import re
from pathlib import Path

from generate_public_routes import triple

EXP = Path(__file__).resolve().parents[1]
SOURCE = EXP / 'research/refresh_1010/notebooks/tokenjunkielabs/titan-kaggriculture-frontier-source'
TARGET = EXP / 'league/titan_frontier'


def main():
    routes = json.loads((SOURCE / '_V43_ROUTES.json').read_text())
    offsets, values, rows = [], [], []
    for name in ('default', 'yarn_first', 'yarn_second'):
        route = routes[name]
        assert len(route) == 719
        row = []
        for action in route:
            row.append(len(values))
            units = [action['farmer'], *action['hands']]
            values += [len(units), len(action['market'])]
            for unit in units:
                values.extend(triple(unit))
            for order in action['market']:
                values.extend(triple(order, True))
        offsets.append(row)
        rows.append({'name': name, 'actions': len(route), 'canonical_sha256': hashlib.sha256(json.dumps(route, separators=(',', ':'), sort_keys=True).encode()).hexdigest()})
    code = '// Apache-2.0 public v43 route data; exact lineage in ../IMPORT.json.\n'
    code += 'inline constexpr int route_offsets[3][719]={\n' + ',\n'.join('{' + ','.join(map(str, r)) + '}' for r in offsets) + '\n};\n'
    code += 'inline constexpr int route_values[]={\n' + ',\n'.join(','.join(map(str, values[i:i + 100])) for i in range(0, len(values), 100)) + '\n};\n'
    (TARGET / 'source/routes.inc').write_text(code)
    notes = (SOURCE / 'notes.md').read_text()
    for name in ('LICENSE', 'NOTICE.txt'):
        match = re.search(r'## ' + re.escape(name) + r'\n\nSHA256 `([a-f0-9]+)`\n\n````\n(.*?)\n````', notes, re.S)
        assert match
        body = match.group(2)
        alternatives = [body, body + '\n', body.lstrip('\n'), body.lstrip('\n') + '\n']
        exact = next(text for text in alternatives if hashlib.sha256(text.encode()).hexdigest() == match.group(1))
        (TARGET / name).write_text(exact)
    modules = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((SOURCE / 'modules').glob('*.py'))}
    report = {'source': str((SOURCE / 'extracted_main.py').relative_to(EXP)),
              'source_url': 'https://www.kaggle.com/code/tokenjunkielabs/titan-kaggriculture-frontier-source',
              'source_sha256': hashlib.sha256((SOURCE / 'extracted_main.py').read_bytes()).hexdigest(),
              'pinned_repository': 'https://github.com/woahwhattheheck/commons/tree/cf88ace1ba45a170fb8ba09986b5f7cb1413381b/revenue/kaggriculture/cloud-frontier-policy',
              'authors': ['Kaito Fukami: complete v43 routes and sparse shop/weed/SELL controller', 'Igor Zharov: selected legal observation, projected inventory, and price-impact helpers', 'LARK / Commons: finished-product sale advance for public farm signature distance<=2'],
              'license': 'Apache-2.0; exact LICENSE and NOTICE.txt copied from notebook',
              'modules': modules, 'routes': rows,
              'changes': ['C++ local API and immutable numeric route tables', 'All three independent child weed controllers updated from turn zero', 'Preserve literal sale-quantity and same-turn projected-shed behavior, including zero-quantity duplicate SELL slots'],
              'inactive_source': ['Disabled bakery market-maker; step-zero call only resets it', 'Unused terminal, quantity MPC, simulator, route library and market-maker alternatives'],
              'approximate_rating': None, 'rating_note': 'No actual leaderboard rating claimed by pinned package; notebook evaluation claims not treated as current strength',
              'status': 'Port pending original-source parity and complete-game screen'}
    (TARGET / 'IMPORT.json').write_text(json.dumps(report, indent=2) + '\n')
    (TARGET / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'titan_frontier', 'header': 'source/agent.hpp', 'type': 'compositions::titan_frontier::Agent', 'sources': ['source/agent.cpp']}, indent=2) + '\n')
    parent = EXP / 'league/kaito_v43'
    (parent / 'source').mkdir(parents=True, exist_ok=True)
    (parent / 'source/agent.hpp').write_text('#pragma once\n#include "../../titan_frontier/source/agent.hpp"\nnamespace compositions::kaito_v43 {class Agent:public titan_frontier::AgentCore {public:Agent():AgentCore(false){}static kag::agent::AgentInfo info(){return {"kaito_v43"};}};}\n')
    (parent / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (parent / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'kaito_v43', 'header': 'source/agent.hpp', 'type': 'compositions::kaito_v43::Agent', 'sources': ['source/agent.cpp', '../titan_frontier/source/agent.cpp']}, indent=2) + '\n')
    (parent / 'README.md').write_text('# Kaito v43 baseline\n\nThe three-route sparse shop controller from TITAN, with the LARK sale overlay disabled. This is a causal baseline for the overlay. Full Apache-2.0 source and module lineage are in `../titan_frontier/IMPORT.json`. Rating is unknown; parity and screening pending.\n')
    print('routes', len(rows), 'integers', len(values))


if __name__ == '__main__':
    main()
