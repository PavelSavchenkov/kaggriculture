"""Audit frozen notebooks on exogenous observations, never Python gameplay."""
import ast
import base64
import hashlib
import importlib.util
import json
import zlib
from collections import Counter
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
FRESH = EXP / 'research/refresh_1010/notebooks'


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def blob_sha(path):
    tree = ast.parse(path.read_text())
    node = next(n for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == '_BLOB' for t in n.targets))
    return hashlib.sha256(zlib.decompress(base64.b64decode(ast.literal_eval(node.value)))).hexdigest()


def main():
    ttv = FRESH / 'kunaldesale2408/kaggriculture-ttv1'
    old = EXP / 'research/refresh_0504/notebooks/lynnsakurai/farming-score-a-mathematical-approach/extracted_MAIN_B64.py'
    new = ttv / 'extracted_MAIN_B64.py'
    assert new.read_bytes() == old.read_bytes()
    report = {'source_url': 'https://www.kaggle.com/code/kunaldesale2408/kaggriculture-ttv1',
              'source_sha256': sha(new), 'same_bytes_as': str(old.relative_to(EXP)),
              'same_existing_cpp_agent': 'league/public_terminal_router',
              'novelty': 'None relative to the existing full Lynn terminal port. Relative to public_capacity_router alone, adds final-step projected-stock liquidation, already preserved by AgentCore(true).',
              'decoded_thomas_blob_sha256': blob_sha(new),
              'component_lineage': ['Thomas Tschinkel four prefix-compatible public routes, weed/no-op and dead-stock handling', 'tetsutani end-of-day shed-capacity guard', 'Lynn Sakurai final-step projected-stock liquidation', 'TTV1 re-publication: byte-identical standalone source'],
              'existing_validation': 'results/capacity_router_parity.json and the existing public-terminal/capacity complete-game validation files; exact source identity transfers the source-parity evidence',
              'rating': None, 'decision': 'Reuse existing public_terminal_router; no duplicate league entry or second port'}
    (ttv / 'AUDIT.json').write_text(json.dumps(report, indent=2) + '\n')
    directory = FRESH / 'y3uanm/kaggriculture-market-impact-router-v4'
    source = directory / 'extracted_main.py'
    thomas = EXP / 'research/notebooks/thomastschinkel/kaggriculture-public-state-router-74-5-win-rate/extracted_main.py'
    module = load(source, 'market_v4_audit')
    parent = load(thomas, 'thomas_audit')
    counts = Counter()
    minima = {item: 10**9 for item in module.PRODUCTS}
    maximum_projected_quote_inventory = {item: 0 for item in module.PRODUCTS}
    positive_products = Counter()
    examples = []
    rank = module._rank_sell_orders
    current = None
    def traced(market, prices, inventory):
        counts['ranking_calls'] += 1
        valid = [o for o in market if o and o[0] == 'SELL' and len(o) >= 3 and o[1] in module._MARKET_PARAMS]
        counts['calls_with_multiple_sales'] += len(valid) >= 2
        for order in valid:
            item, quantity = order[1], max(0, int(order[2]))
            projected = int(inventory.get(item, 10000) or 0)
            later = module._market_price(item, projected + quantity)
            actual = prices.get(item) or module._market_price(item, projected)
            positive = quantity * max(0, actual - later) > 0
            counts['positive_impact_rows'] += positive
            positive_products[item] += positive
            if positive and len(examples) < 12:
                examples.append({'item': item, 'quantity': quantity, 'projected_own_shed': projected, 'current_quote': actual, 'predicted_later_quote': later})
            maximum_projected_quote_inventory[item] = max(maximum_projected_quote_inventory[item], projected + quantity)
        result = rank(market, prices, inventory)
        counts['ranking_changes'] += result != market
        return result
    module._rank_sell_orders = traced
    available = sorted((EXP / 'replays').glob('*replay.json'))
    paths = available[:8] + available[-16:]
    paths = list(dict.fromkeys(paths))
    for path in paths:
        replay = json.loads(path.read_text())
        for seat in range(2):
            a, b = module.Agent(), parent.Agent()
            for step, pair in enumerate(replay['steps'][:719]):
                obs = dict(pair[seat]['observation'], step=step)
                for item in minima:
                    minima[item] = min(minima[item], obs['market']['inventory'][item])
                actual, expected = a.act(obs), b.act(obs)
                counts['action_changes_vs_thomas'] += actual != expected
                counts['actions'] += 1
    # A counterexample to a universal no-op claim. Shared inventory zero is a
    # synthetic helper input, not asserted reachable in the normal full game.
    example_orders = [['SELL', 'MILK', 1], ['HIRE'], ['SELL', 'WHEAT', 99]]
    example_projected = {'MILK': 1, 'WHEAT': 99}
    example_prices = {item: module._market_price(item, 0) for item in ('MILK', 'WHEAT')}
    changed = rank(example_orders, example_prices, example_projected)
    assert changed != example_orders
    report = {'source_url': 'https://www.kaggle.com/code/y3uanm/kaggriculture-market-impact-router-v4',
              'source_sha256': sha(source), 'parent_source_sha256': sha(thomas),
              'decoded_thomas_blob_sha256': blob_sha(source),
              'novelty': 'Ranks existing SELL slots by quantity times predicted price loss; non-SELL slots remain in position. The caller supplies projected own shed instead of shared market inventory. Its CARROT/TOMATO/EGG below-equilibrium curves are older than the official hinge curves.',
              'evidence': counts, 'replay_files': [str(p.relative_to(EXP)) for p in paths],
              'minimum_observed_shared_inventory': minima, 'maximum_projected_shed_plus_quantity': maximum_projected_quote_inventory,
              'zero_impact_condition': 'For every ranked sale, current_quote <= fixed_curve_price(projected_own_shed[item] + quantity) gives zero impact; then stable ranking preserves the parent action. This is conditional and does not hold universally on the recorded observations.',
              'positive_impact_products': positive_products, 'positive_impact_examples': examples,
              'non_universal_counterexample': {'scope': 'Synthetic low-shared-inventory helper inputs; reachability in a full normal game not claimed', 'market_before': example_orders, 'prices': example_prices, 'projected_own_shed': example_projected, 'market_after': changed},
              'rating': None, 'decision': 'Preserve literal behavior in a separate C++ port and run a small complete-game screen. Do not treat source bug or small action count as a measured performance conclusion.'}
    (directory / 'AUDIT.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'ttv_exact_duplicate': True, 'market_v4': counts, 'counterexample': changed}, indent=2))


if __name__ == '__main__':
    main()
