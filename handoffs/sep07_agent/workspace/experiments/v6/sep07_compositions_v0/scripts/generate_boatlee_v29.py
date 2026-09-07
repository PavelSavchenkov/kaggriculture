"""Static extraction and immutable C++ formatting of the audited V29 source."""
import ast
import base64
import hashlib
import json
import zlib
from pathlib import Path

from generate_public_routes import triple

EXP = Path(__file__).resolve().parents[1]
DONOR = EXP / "research/notebooks/boatlee/v29-r1-adaptive-market-hysteresis"
TARGET = EXP / "league/boatlee_v29"


def main():
    raw = (DONOR / "extracted_main.py").read_bytes()
    tree = ast.parse(raw)
    assignments = {node.targets[0].id: node.value for node in tree.body
                   if isinstance(node, ast.Assign) and isinstance(node.targets[0], ast.Name)}
    encoded = next(node.value for node in ast.walk(assignments["_ACTIONS"])
                   if isinstance(node, ast.Constant) and isinstance(node.value, (str, bytes)) and len(node.value) > 400)
    decoded = zlib.decompress(base64.b85decode(encoded))
    actions = json.loads(decoded)
    config = ast.literal_eval(assignments["_AM_CONFIG"])
    assert len(actions) in (719, 720) and ast.literal_eval(assignments["_FR_ITEMS"]) == ()
    offsets, values = [], []
    for action in actions:
        offsets.append(len(values))
        units = [action["farmer"], *action["hands"]]
        values.extend([len(units), len(action["market"])])
        for unit in units:
            values.extend(triple(unit))
        for order in action["market"]:
            values.extend(triple(order, True))
    code = '// Generated from frozen V29-R1 source; see IMPORT.json.\n'
    code += 'inline constexpr int offsets[]={' + ','.join(map(str, offsets)) + '};\n'
    code += 'inline constexpr int data[]={\n' + ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + '\n};\n'
    code += 'namespace config {\n' + ''.join(f'inline constexpr auto {k}={v};\n' for k, v in config.items()) + '}\n'
    (TARGET / "source").mkdir(parents=True, exist_ok=True)
    (TARGET / "source/data.inc").write_text(code)
    (DONOR / "ACTIONS.json").write_bytes(decoded)
    (DONOR / "CONFIG.json").write_text(json.dumps(config, indent=2) + '\n')
    manifest = {"format_version": 1, "name": "boatlee_v29", "header": "source/agent.hpp",
                "type": "compositions::boatlee_v29::Agent", "sources": ["source/agent.cpp"]}
    (TARGET / "agent.json").write_text(json.dumps(manifest, indent=2) + '\n')
    lineage = {"source": "https://www.kaggle.com/code/boatlee/v29-r1-adaptive-market-hysteresis",
               "source_sha256": hashlib.sha256(raw).hexdigest(), "actions_sha256": hashlib.sha256(decoded).hexdigest(),
               "data_sha256": hashlib.sha256(code.encode()).hexdigest(), "config": config,
               "production_origin": {"team": "RngRng", "submission": 55948382, "episodes_credited_by_author": [104679155, 104689164, 104691666]},
               "changes": "Typed C++ implementation of fixed course, eight-step weed recovery and adaptive market controller; empty front-run item list makes legacy front-run/debt code inactive.",
               "rating": "Not independently established. Notebook reports strong local top20/regression panels, not verified by us yet.",
               "reuse": "User explicitly authorized borrowing public agents. Separate source license not supplied in downloaded metadata; source attribution retained.",
               "status": "Port under construction; parity and competitive/deployment checks pending."}
    (TARGET / "IMPORT.json").write_text(json.dumps(lineage, indent=2) + '\n')
    (TARGET / "README.md").write_text('# boatlee_v29\n\nFaithful experimental C++ port of Boatlee V29-R1. RngRng-derived complete production course, eight-step weed recovery and stateful premium sales using public residual supply, inventory reserves, demand, mirror guard and finite sale budgets. Per-instance state; legal observations only. Exact source and original episode credits in IMPORT.json. Source parity, competitive and deployment checks pending.\n')
    print('actions', len(actions), 'integers', len(values))


if __name__ == "__main__":
    main()
