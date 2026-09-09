from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
opponents = ['empty_sale_slots_m2','teammate_shoprouter','public_router','public_router_v52','ahmed_v23','pass']
rows = []
for opponent in opponents:
    get = lambda name: json.loads((RUN/'discovery'/f'{name}_vs_{opponent}.json').read_text())
    original, control, candidate = [get(n) for n in ['public_router_v52','farm_signal_v52_m0','farm_signal_v52_m1']]
    assert original['games'] == control['games']
    old = {(g['seed'],g['seat']):g for g in original['games']}
    pairs = [(g,old[g['seed'],g['seat']]) for g in candidate['games']]
    assert len(pairs) == 64
    mean_delta = lambda fn: statistics.mean(fn(a)-fn(b) for a,b in pairs)
    wtl = lambda gs: [sum((g['cash']>g['opponent_cash'],g['cash']==g['opponent_cash'],g['cash']<g['opponent_cash'])[i] for g in gs) for i in range(3)]
    row = {'opponent':opponent,'games':len(pairs),'parent_wtl':wtl(original['games']),'candidate_wtl':wtl(candidate['games']),
        'own_cash_gain':mean_delta(lambda g:g['cash']),'margin_gain':mean_delta(lambda g:g['cash']-g['opponent_cash']),
        'fault_gain':mean_delta(lambda g:g['unit_faults']),
        'discard_gain':[mean_delta(lambda g:g['discarded'][i]) for i in range(9)],
        'production_gain':[mean_delta(lambda g:g['produced'][i]) for i in range(9)],
        'hires_gain':mean_delta(lambda g:g['profile']['hires']),
        'hire_cost_gain':mean_delta(lambda g:g['profile']['hire_cost']),
        'exact_full_records':sum(a==b for a,b in pairs),
        'physical_records_equal':sum(all(a[k]==b[k] for k in ['produced','discarded','unit_faults','worker_days']) for a,b in pairs),
        'candidate_seconds':candidate['seconds'],'parent_seconds':original['seconds']}
    rows.append(row)
report = {'discovery_games':1152,'disabled_control_exact_games':384,'rows':rows,
    'guard_parity':json.loads((RUN/'GUARD_PARITY.json').read_text()),
    'operational':json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text()),
    'selection_scope':'Discovery of a faithful public component. This does not authorize a global promotion or replace a fresh audit.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text = '# Farm Signal capacity discovery\n\n1800 pure-oracle cases and384 full disabled-control games match exactly. All36 required operational games pass. The public source and C++ policy preserve the same approximate requested-action accounting.\n\n| Opponent | Parent W/T/L | Candidate W/T/L | Own cash gain | Margin gain | Fault gain | Discard gain |\n| --- | ---: | ---: | ---: | ---: | ---: | ---: |\n'
for r in rows:
    text += f"| {r['opponent']} | {r['parent_wtl']} | {r['candidate_wtl']} | {r['own_cash_gain']:+.2f} | {r['margin_gain']:+.2f} | {r['fault_gain']:+.3f} | {sum(r['discard_gain']):+.3f} |\n"
text += '\nThese exposed discovery seeds cannot establish a fresh strength claim. See ANALYSIS.json for physical-output, labor and throughput details.\n'
(RUN/'RESULTS.md').write_text(text)
print(text)
