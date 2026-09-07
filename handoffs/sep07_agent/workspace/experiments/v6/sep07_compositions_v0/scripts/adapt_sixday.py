"""Copy and mechanically adapt the published C++ policy; no Python gameplay."""
import hashlib
import json
import re
import shutil
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
ORIGIN = ROOT / "external/kaggriculture/agents/sixday-rl"
TARGET = EXP / "league/teammate_sixday"
UPSTREAM = TARGET / "source/upstream"
UPSTREAM.mkdir(parents=True, exist_ok=True)
hashes = {}
for relative in ("source/policy.cpp", "source/six_day_budget_guard.hpp", "source/source_manifest.json", "main.py", "_sixday_main.py", "NOTICE", "BUILD.md"):
    data = (ORIGIN / relative).read_bytes()
    hashes[relative] = hashlib.sha256(data).hexdigest()
    destination = UPSTREAM / ("policy.txt" if relative == "source/policy.cpp" else Path(relative).name)
    destination.write_bytes(data)
shutil.copy2(ORIGIN / "NOTICE", TARGET / "NOTICE")
(TARGET / "IMPORT.json").write_text(json.dumps({"origin": str(ORIGIN.relative_to(ROOT)), "sha256": hashes,
    "notebook": "https://www.kaggle.com/code/yhay81/six-day-public-state-fieldbook",
    "dataset": "https://www.kaggle.com/datasets/yhay81/six-day-public-state-agent-source",
    "license": "Apache-2.0 backbone; teammate's market head and reserve repair retain their original attribution",
    "adaptation": "Persistent engine types, isolated guard namespace, immutable shared decoded tables, per-instance route state, C++ translation of Python wrappers"}, indent=2)+"\n")
(UPSTREAM / "runtime_types.hpp").write_text('#pragma once\n#include "fast_game_engine/sim.hpp"\n')
guard = (ORIGIN / "source/six_day_budget_guard.hpp").read_text()
guard = guard.replace('#include "sim.hpp"', '#include "runtime_types.hpp"').replace("kag::native", "kag::sixday_native")
(UPSTREAM / "six_day_budget_guard.hpp").write_text(guard)
source = (ORIGIN / "source/policy.cpp").read_text().split('extern "C"')[0]
source = source.replace('#include "policy_plugin_abi.hpp"', '#include "runtime_types.hpp"')
source = source.replace('#include <stdexcept>', '#include <cstdlib>')
source = re.sub(r'throw std::runtime_error\("[^"]+"\);', 'std::abort();', source)
source = source.replace("kag::native", "kag::sixday_native")
start = source.index("struct Context {")
action_at = source.index("    kag::Action action_for", start)
old = source[start:action_at]
fields = old[old.index("    std::array"):old.index("    int selected_")]
constructor = old[old.index("    Context() {"):]
constructor = constructor.replace("    Context() {", "        Tables() {")
source = source[:start] + "struct Context {\n    int selected_[5]{};\n    struct Tables {\n" + fields + constructor + "    };\n    static const Tables& tables() {\n        static const Tables value;\n        return value;\n    }\n\n" + source[action_at:]
source = re.sub(r"return segment_([0-4])\[", r"return tables().segment_\1[", source)
(UPSTREAM / "policy.hpp").write_text(source)
catalog_path = EXP / "configs/league.json"
catalog = json.loads(catalog_path.read_text())
for name, mode in (("teammate_sixday", 1), ("teammate_sixday_robust", 2), ("public_sixday", 0)):
    target = EXP / "league" / name
    (target / "source").mkdir(parents=True, exist_ok=True)
    if name != "teammate_sixday":
        (target / "source/agent.hpp").write_text('#pragma once\n#include "../../teammate_sixday/source/agent.hpp"\n'
            f'namespace compositions::{name} {{\nclass Agent:public teammate_sixday::AgentCore {{\npublic:\n'
            f'Agent():AgentCore({mode}) {{}}\nstatic kag::agent::AgentInfo info() {{return {{"{name}"}};}}\n}};\n}}\n')
        (target / "source/agent.cpp").write_text('#include "agent.hpp"\n')
        shutil.copy2(ORIGIN / "NOTICE", target / "NOTICE")
    manifest = {"format_version": 1, "name": name, "header": "source/agent.hpp", "type": f"compositions::{name}::Agent",
        "sources": ["source/agent.cpp"] + ([] if name == "teammate_sixday" else ["../teammate_sixday/source/agent.cpp"])}
    (target / "agent.json").write_text(json.dumps(manifest, indent=2)+"\n")
    (target / "README.md").write_text(f"# {name}\n\nC++ adaptation of yhay81's Six-Day Public-State Fieldbook; "
        f"wrapper mode {mode} (0 published backbone, 1 teammate selling head, 2 teammate $150 purchase reserve and price-floor selling guard). "
        "The source owns complete 144-turn segment routes, public-feature decision trees, and segment financing. "
        "All borrowed code attribution and hashes are in teammate_sixday/IMPORT.json and NOTICE. "
        "The reserve wrapper comes from external/kaggriculture/agents/sixday-robust-rl/main.py; its source copy and hash are in this package.\n\n"
        "Strength, parity and optimization status: untested port until results are recorded. Source-reported performance is not a current Kaggle rating. "
        "No strategic optimization in this adaptation. State is reset per instance; immutable decoded routes are shared. "
        "Only legal public observations and own inventories are supplied to the native policy.\n")
    catalog[name] = str(target.relative_to(ROOT))
robust_source = ROOT / "external/kaggriculture/agents/sixday-robust-rl/main.py"
robust_target = EXP / "league/teammate_sixday_robust"
shutil.copy2(robust_source, robust_target / "original_main.py")
(robust_target / "IMPORT.json").write_text(json.dumps({"source": str(robust_source.relative_to(ROOT)),
    "sha256": hashlib.sha256(robust_source.read_bytes()).hexdigest(), "backbone": "../teammate_sixday/IMPORT.json"},indent=2)+"\n")
catalog_path.write_text(json.dumps(catalog, indent=2)+"\n")
print("Adapted six-day backbone and registered three wrapper modes")
