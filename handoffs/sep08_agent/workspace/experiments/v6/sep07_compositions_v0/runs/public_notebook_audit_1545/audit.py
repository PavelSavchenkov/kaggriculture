"""Static source/lineage audit. No downloaded source is imported or executed."""
import ast
import base64
import copy
import hashlib
import json
import shutil
import zlib
from datetime import datetime, timezone
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name, data):
    (RUN / name).write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")


def parse(path):
    text = path.read_text()
    return ast.parse("\n".join(line for line in text.splitlines() if not line.startswith(("%", "!"))))


def payload(tree):
    node = next(n for n in tree.body if isinstance(n, ast.Assign) and isinstance(n.targets[0], ast.Tuple))
    literal = ast.literal_eval(node.value.args[0].args[0].args[0])
    return zlib.decompress(base64.b64decode(literal))


class BlankStrings(ast.NodeTransformer):
    def visit_Constant(self, node):
        if isinstance(node.value, str):
            return ast.copy_location(ast.Constant(value=""), node)
        return node


def main():
    refs = {
        "orders_previous.ipynb": EXP / "research/refresh_0504/notebooks/az05192000gmailcom/kaggriculture-from-orders-to-actual-trades/kaggriculture-from-orders-to-actual-trades.ipynb",
        "igor_previous.ipynb": EXP / "research/refresh_1342/notebooks/flexonafft/kaggriculture-smart-farm-strategy-lab/kaggriculture-smart-farm-strategy-lab.ipynb",
        "v5_reference.py.txt": EXP / "league/public_router_v5/upstream_reference.py.txt",
        "v5_IMPORT.json": EXP / "league/public_router_v5/IMPORT.json",
        "v5_validation.json": EXP / "results/public_router_v5_validation.json",
        "capacity_IMPORT.json": EXP / "league/public_capacity_router/IMPORT.json",
        "notebooks_by_date.csv": EXP / "research/refresh_1545/notebooks_by_date.csv",
        "notebook_changes.json": EXP / "research/refresh_1545/notebook_changes.json",
        "leaderboard.csv": EXP / "research/refresh_1545/kaggriculture-publicleaderboard-2026-09-07T15:48:50.csv",
    }
    reference = RUN / "reference"
    reference.mkdir(exist_ok=True)
    inputs = {}
    for name, path in refs.items():
        target = reference / name
        shutil.copyfile(path, target)
        inputs[name] = {"original_path": str(path.relative_to(EXP)), "copy": str(target.relative_to(RUN)), "sha256": sha(target)}

    old_orders = parse(RUN / "orders/previous_static/code.py.txt")
    new_orders = parse(RUN / "orders/static/code.py.txt")
    assert ast.dump(BlankStrings().visit(copy.deepcopy(old_orders))) == ast.dump(BlankStrings().visit(copy.deepcopy(new_orders)))
    old_constants = [n.value for n in ast.walk(old_orders) if isinstance(n, ast.Constant) and isinstance(n.value, str)]
    new_constants = [n.value for n in ast.walk(new_orders) if isinstance(n, ast.Constant) and isinstance(n.value, str)]
    assert len(old_constants) == len(new_constants)
    string_changes = [{"old": a, "new": b} for a, b in zip(old_constants, new_constants) if a != b]
    old_functions = {n.name: n for n in old_orders.body if isinstance(n, ast.FunctionDef)}
    new_functions = {n.name: n for n in new_orders.body if isinstance(n, ast.FunctionDef)}
    assert old_functions.keys() == new_functions.keys()
    functions = [{"function": name, "ast_equal": ast.dump(old_functions[name]) == ast.dump(node)} for name, node in new_functions.items()]
    assert [f["function"] for f in functions if not f["ast_equal"]] == ["market_row"]
    orders = {
        "ref": "az05192000gmailcom/kaggriculture-from-orders-to-actual-trades",
        "author": "3정훈", "kernel_id": 133380514, "last_run_from_listing": "2026-09-07 15:18:37.353000",
        "classification": "Existing replay accounting notebook; cosmetic/documentation changes only",
        "all_non_string_ast_equal": True, "function_comparisons": functions, "changed_string_constants": string_changes,
        "new_agent": False,
        "demo_scope": "One goose plus CARROT/WHEAT/TOMATO; simple prioritized movement, up to one morning hire, fixed sale batch3; author explicitly says noncompetitive demo",
        "reusable_components": [
            {"function": "replay_market_turn", "idea": "Reconstruct filled trades one item at a time at corresponding order slots; quote both seats before commits, preserve abandoned slots and price-floor inventory behavior", "scope": "Offline evidence only; unchanged from prior version"},
            {"function": "shed_before_market", "idea": "Apply same-turn unit actions before valuing trades; respect all-or-none seed oversubscription and shared tile mutations", "scope": "Requires both full private states and compatible internal engine API"},
            {"function": "autopsy", "idea": "Report signed, absolute, worst-turn and mismatch-count cash residuals", "scope": "Zero net-cash error does not prove each product fill; retain production and inventory reconciliation"},
        ],
        "limitations": [
            "No new competitive composition, crop calendar, placement logic, shop branch or donor private agent source is supplied",
            "Feed category labels all bought wheat, including wheat later resold; sales are not production",
            "Curves use default prices and omit production/worker costs and opponent counterfactuals; they are not composition ROI estimates",
            "Unit helper depends on installed kaggle-environments internals; notebook states tested1.32.7",
            "Notebook cells were not run; accounting examples were inspected, not independently revalidated in this audit",
        ],
    }
    save("ORDERS_AUDIT.json", orders)

    agent_path = RUN / "igor/static/extracted_AGENT_SOURCE.py"
    current = parse(agent_path)
    previous_v5 = parse(reference / "v5_reference.py.txt")
    raw = payload(current)
    assert raw == payload(previous_v5)
    assert isinstance(current.body[0], ast.Expr) and isinstance(current.body[0].value, ast.Constant)
    current.body = current.body[1:]
    previous_v5.body = previous_v5.body[1:]
    assert ast.dump(current) == ast.dump(previous_v5)
    tapes, trees = json.loads(raw)
    assert len(tapes) == 5 and all(len(t) == 719 for t in tapes)
    (RUN / "igor/static/decoded.json").write_bytes(raw)
    v5 = json.loads((reference / "v5_IMPORT.json").read_text())
    validation = json.loads((reference / "v5_validation.json").read_text())
    for rel, expected in validation["policy_source_sha256"].items():
        path = EXP.parents[2] / rel
        assert sha(path) == expected, path
    igor = {
        "ref": "flexonafft/kaggriculture-smart-farm-strategy-lab", "author": "Igor Zharov",
        "kernel_id": 133331413, "last_run_from_listing": "2026-09-07 15:08:57.043000",
        "new_source_sha256": sha(agent_path),
        "previous_source_sha256": sha(RUN / "igor/previous_static/extracted_MAIN_B64.py"),
        "classification": "Replaces old capacity router with already ported Thomas public_router_v5; not a new phenotype",
        "all_module_ast_equal_after_removing_only_leading_docstring": True,
        "payload_byte_equal": True, "payload_sha256": hashlib.sha256(raw).hexdigest(),
        "tapes": 5, "actions": 3595, "trees": trees,
        "reference_agent": "league/public_router_v5",
        "reference_source_sha256": inputs["v5_reference.py.txt"]["sha256"],
        "reference_source_url": "https://www.kaggle.com/code/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router",
        "effective_rules": v5["effective_features"],
        "source_exact_details": [
            "Select tape0 at0,288,432; at144 select4 if Yarn count>0, else2 if milk demand>0, else1; at576 select3 if carrot quote<=54, else0",
            "Computed opponent/cash/farm features are unused by these decision nodes; no explicit opponent adaptation is present",
            "No terminal repair, physical continuation guard, funding repair or stock-based sale adjustment is added",
            "Mutable Python _SESSIONS is indexed by player; existing C++ port instead stores independent per-instance state and normalizes active worker count per local API",
        ],
        "lineage_limit": "Same code and data prove equivalence, not which author first created the controller. Both versions mention missing provenance.json; original replay source mapping remains unknown. Nearest-tape comparisons in copied V5 IMPORT are measured similarities, not proof of authorship.",
        "reuse_permission": "Public source; user explicitly authorizes borrowing. Downloaded metadata contains no separate license.",
        "existing_validation": {
            "source_parity": validation["source_parity"],
            "generic_debug_thread_full_records_identical": validation["generic_debug_thread_full_records_identical"],
            "pass_games": validation["pass"]["games"], "self_games": validation["self"]["games"],
            "cpp_policy_hashes_still_match": True,
            "scope": "Prior evidence inherited by exact source/data equivalence; this audit ran no new C++ matches and makes no new win-rate claim",
        },
        "decision": "Reuse existing validated public_router_v5 and its lineage; no duplicate C++ agent or new seed screen",
    }
    save("IGOR_AUDIT.json", igor)
    save("LINEAGE.json", {"timestamp_utc": datetime.now(timezone.utc).isoformat(), "inputs": inputs,
                          "fresh_extraction": "EXTRACTION.json", "static_only": True,
                          "new_ports": 0, "new_gameplay_runs": 0, "fresh_seed_pool1720000_consumed": False})
    print(json.dumps({"orders_functions": len(functions), "orders_unchanged_functions": sum(f["ast_equal"] for f in functions),
                      "changed_string_constants": len(string_changes), "igor_all_module_ast_equal": True,
                      "igor_payload_equal": True, "new_ports": 0}, indent=2))


if __name__ == "__main__":
    main()
