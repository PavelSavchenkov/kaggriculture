from datetime import datetime, timezone
from pathlib import Path
import json

RUN = Path(__file__).resolve().parent
parent = 'observed_sale_lead_start_216'
traces = {}
endpoints = {}
for mode, name in [(0, parent), (2, 'empty_sale_slots_m2')]:
    stem = RUN / f'trace_results/m{mode}_2004097_s0'
    endpoint = json.loads(stem.with_suffix('.json').read_text())
    records = json.loads((RUN / f'native/{name}_vs_{parent}.json').read_text())['games']
    expected = next(g for g in records if g['seed'] == 2004097 and g['seat'] == 0)
    for key in ['cash', 'opponent_cash', 'action_hash', 'opponent_action_hash']:
        assert endpoint[key] == expected[key], (mode, key, endpoint[key], expected[key])
    traces[mode] = [json.loads(line) for line in stem.with_suffix('.jsonl').read_text().splitlines()]
    assert len(traces[mode]) == 719
    assert [t['step'] for t in traces[mode]] == list(range(719))
    endpoints[mode] = endpoint
changes = []
same_state_effects = []
negative_path_increments = []
previous_gain = [0, 0]
first_inventory_divergence = None
for old, new in zip(traces[0], traces[2]):
    assert old['step'] == new['step']
    if old['own_orders'] != new['own_orders'] or old['rival_orders'] != new['rival_orders']:
        changes.append({'step': old['step'], 'old_own': old['own_orders'], 'new_own': new['own_orders'],
                        'old_rival': old['rival_orders'], 'new_rival': new['rival_orders']})
    gain = [a-b for a, b in zip(new['cash_after'], new['cash_raw_order_counterfactual'])]
    if gain != [0, 0]:
        same_state_effects.append({'step': new['step'], 'cash_gain': gain, 'margin_gain': gain[0]-gain[1],
                                  'raw_orders': new['raw_orders'], 'own_orders': new['own_orders'],
                                  'rival_orders': new['rival_orders'], 'inventory_before': new['inventory']})
    path_gain = [a-b for a, b in zip(new['cash_after'], old['cash_after'])]
    increment = [a-b for a, b in zip(path_gain, previous_gain)]
    if increment[0]-increment[1] < 0:
        negative_path_increments.append({'step': new['step'], 'cash_increment': increment,
            'old_orders': old['own_orders'], 'new_orders': new['own_orders'], 'rival_orders': new['rival_orders']})
    previous_gain = path_gain
    if first_inventory_divergence is None and old['inventory'] != new['inventory']:
        first_inventory_divergence = {'step': old['step'], 'old': old['inventory'], 'new': new['inventory']}
cash_gain = [endpoints[2][key]-endpoints[0][key] for key in ['cash', 'opponent_cash']]
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'full_games': 2,
          'endpoint_cash_and_both_action_hashes_exact': True,
          'seed': 2004097, 'seat': 0, 'cash_gain': cash_gain, 'margin_gain': cash_gain[0]-cash_gain[1],
          'rival_orders_identical_every_step': all(a['rival_orders'] == b['rival_orders'] for a, b in zip(traces[0], traces[2])),
          'changed_steps': changes, 'same_state_immediate_effects': same_state_effects,
          'negative_same_state_margin_effects': [effect for effect in same_state_effects if effect['margin_gain'] < 0],
          'first_inventory_divergence': first_inventory_divergence,
          'negative_path_increments': negative_path_increments,
          'finding': 'The first order change at270 has no economic effect in this case. The causal change is at598: removing an empty milk sale advances11 strawberries. Both players otherwise sell11 strawberries on the same slot. The floor-price rule only increments market inventory when the quote exceeds1. Sequential sales leave strawberry inventory one lower than simultaneous sales (10062 versus10063). Immediate rival cash falls2, but subsequent unchanged trades pay more to both players, ultimately +47 own/+53 rival. Input movement is not the cause of this failed case.',
          'next_hypothesis': 'Preserve empty slots when a later shifted sale can reach the price floor. Test guards using own quantity, a matched rival-quantity assumption, and the rival shed-capacity bound. These are observation-based economic conditions, not seed or opponent exclusions. A tail-only non-input rule would not prevent the demonstrated strawberry failure.',
          'scope': 'Both complete native runs match the frozen audit exactly. Each counterfactual clones the actual current simulator and uses the same rival/unit actions, replacing only our compressed orders with the inherited raw orders. It isolates one-step cash effects; it does not assume those effects simply add across changed future states.'}
assert cash_gain == [47, 53]
assert first_inventory_divergence['step'] == 599
assert same_state_effects[0]['step'] == 598
(RUN / 'TRACE_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
print('Exact native traces; cash gain', cash_gain, 'margin', report['margin_gain'])
print('First changed orders:', changes[0])
print('Negative same-state immediate effects:', report['negative_same_state_margin_effects'])
