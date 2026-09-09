"""Check the frozen candidate's preregistered gates and promote the local reference."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
CANDIDATE, BASELINE = 'rival_wool_context_v3', 'wool_family_context_v2'


def read(path):
    return json.loads(path.read_text())


def metrics(games):
    margins = [g['cash'] - g['opponent_cash'] for g in games]
    cash = sorted(g['cash'] for g in games)
    tail = math.ceil(len(games) / 10)
    return {'games': len(games), 'wins': sum(m > 0 for m in margins), 'ties': sum(m == 0 for m in margins),
            'utility': statistics.mean(1 if m > 0 else .5 if m == 0 else 0 for m in margins),
            'mean_margin': statistics.mean(margins), 'mean_cash': statistics.mean(cash),
            'cvar10_margin': statistics.mean(sorted(margins)[:tail]),
            'pass_J': .8 * statistics.mean(cash) + .2 * statistics.mean(cash[:tail])}


def contrast(a, b):
    assert [(g['seed'], g['seat']) for g in a] == [(g['seed'], g['seat']) for g in b]
    ma, mb = metrics(a), metrics(b)
    row = {key + '_gain': ma[key] - mb[key] for key in ['utility', 'mean_margin', 'mean_cash', 'cvar10_margin', 'pass_J']}
    for key in ['produced', 'sold', 'discarded']:
        row[key + '_gain'] = [statistics.mean(x[key][i] - y[key][i] for x, y in zip(a, b)) for i in range(12)]
    row['buys_gain'] = [statistics.mean(x['profile']['buys'][i] - y['profile']['buys'][i] for x, y in zip(a, b)) for i in range(12)]
    for key in ['unit_faults', 'worker_days', 'opponent_cash']:
        row[key + '_gain'] = statistics.mean(x[key] - y[key] for x, y in zip(a, b))
    for key in ['hires', 'hire_cost', 'land_cost']:
        row[key + '_gain'] = statistics.mean(x['profile'][key] - y['profile'][key] for x, y in zip(a, b))
    row['changed_own_actions'] = sum(x['action_hash'] != y['action_hash'] for x, y in zip(a, b))
    row['changed_rival_actions'] = sum(x['opponent_action_hash'] != y['opponent_action_hash'] for x, y in zip(a, b))
    row['changed_production'] = sum(x['produced'] != y['produced'] for x, y in zip(a, b))
    return row


def main():
    fresh = read(RUN / 'FRESH_ANALYSIS.json')
    checks, final = read(RUN / 'CHECKS.json'), read(RUN / 'FINAL_AUDITS.json')
    spec, frozen = read(RUN / 'FRESH_PREREGISTERED.json'), read(RUN / 'frozen/FROZEN.json')
    amendment = read(RUN / 'PACKAGING_AMENDMENT.json')
    assert all(fresh['gates'].values())
    assert checks['generic_pair_debug_thread_all_records_equal'] and all(checks['additional_branch_changed_games'].values())
    assert final['frozen_full_records_equal'] and final['active_branch_changed_games'] > 0
    for relative, expected in spec['candidate_files_sha256'].items():
        if relative == amendment['path']:
            assert expected == amendment['before_sha256']
            expected = amendment['after_sha256']
        assert hashlib.sha256((EXP / relative).read_bytes()).hexdigest() == expected
    for relative, expected in frozen['files_sha256'].items():
        assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == expected
    native = {}
    plan = read(RUN / 'NATIVE_PLAN.json')
    limits = plan['gates']
    direct_equal = False
    for opponent in dict.fromkeys(job[1] for job in plan['jobs']):
        games = {a: read(RUN / f'native/{a}_vs_{opponent}.json')['games'] for a in [CANDIDATE, BASELINE]}
        assert all(len(g) == 256 and all(row['turns'] == 719 for row in g) for g in games.values())
        row = {'metrics': {a: metrics(g) for a, g in games.items()},
               'contrast': contrast(games[CANDIDATE], games[BASELINE]),
               'full_records_equal': games[CANDIDATE] == games[BASELINE]}
        native[opponent] = row
        assert row['contrast']['mean_margin_gain'] >= limits['minimum_mean_margin_gain']
        assert row['contrast']['utility_gain'] >= limits['minimum_individual_utility_gain']
        if opponent == BASELINE:
            assert all(a['cash']-a['opponent_cash'] >= b['cash']-b['opponent_cash']
                       for a, b in zip(games[CANDIDATE], games[BASELINE], strict=True))
            direct_equal = row['full_records_equal']
    assert native['pass']['contrast']['pass_J_gain'] >= limits['minimum_pass_J_gain']
    witness = read(RUN / 'JOHN_WITNESS.json')['games']
    assert witness == read(EXP / 'runs/rival_wool_validation_002/JOHN_WITNESS.json')['games']
    now = datetime.now(timezone.utc).isoformat()
    report = {'promoted_utc': now, 'candidate': CANDIDATE, 'parent': BASELINE,
              'status': 'Promoted experimental reference after fresh, native, causal, operational and frozen-build checks.',
              'fresh': fresh, 'fresh_record_parity': read(RUN / 'FRESH_RECORD_PARITY.json'),
              'native_and_pass': native, 'operational': checks, 'packaging_amendment': amendment,
              'frozen': {'path': 'runs/rival_wool_validation_003/frozen/FROZEN.json',
                         'dependencies': len(frozen['files_sha256']), 'full_native_records_exact': final['frozen_games']},
              'native_parent_full_records_equal': direct_equal, 'john_failed_sale_witness_fixed': True,
              'lineage': 'runs/rival_wool_context_003/proposals/rival_wool_context_v3/IMPORT.json',
              'diagnosis': 'runs/rival_wool_context_003/PUBLIC_SPENDING_DIAGNOSIS.json',
              'scope': 'Existing early wool choices remain exact. Additional family choices observe public rival spending after a compatible common opening.',
              'limitations': ['Fresh benefit is concentrated against V5/2. Every full record against the other27opponents matches the parent.',
                              'No direct parent gain: the new response does not activate against it. Paired parent-self controls are exactly retained.',
                              'Native V5/2 mean own cash declines$346.21, rival cash declines$516.19, and unit faults increase8.47 per game. The margin gain comes from the shared-market interaction despite worse execution in some transferred farms.',
                              'Net market flows and spending do not identify arbitrary private policies. Different gross trades may produce the same public residual.',
                              'This selects a precompiled whole farm; general composition valuation, placement and independent construction remain incomplete.',
                              'No official catalog copy, Git operation or Kaggle submission.']}
    target = EXP / 'results/rival_wool_context_v3_validation.json'
    target.write_text(json.dumps(report, indent=2) + '\n')
    current = read(EXP / 'CURRENT_REFERENCE.json')
    assert current['name'] == BASELINE
    (RUN / 'PREVIOUS_REFERENCE.json').write_text(json.dumps(current, indent=2) + '\n')
    current.update(name=CANDIDATE, path='runs/rival_wool_context_003/proposals/' + CANDIDATE,
                   promoted_utc=now, report=str(target.relative_to(EXP)), previous=BASELINE)
    (EXP / 'CURRENT_REFERENCE.json').write_text(json.dumps(current, indent=2) + '\n')
    readme = EXP / 'README.md'
    text = readme.read_text()
    start = text.index('Current broad search starting point')
    end = text.index('Previous reference (20:22 UTC):')
    text = text[:start] + f'''Current broad search starting point ({now[11:16]} UTC):
`{current['path']}/`.
Preserve the previous adaptive farm. For additional wool choices, observe rival
hires, net product flows and cash spending one hour after the day6 opening.
Fresh57344 games across28opponents pass all targeted-response gates. V5/2 gains
6.8359 percentage points and$182.14 mean margin; all27 other full match records
are unchanged. Native5120 profiled games,1024 operational games and256 rebuilt
native full records pass. Direct parent remains its exact self-play control.
Read `CURRENT_REFERENCE.json` and `results/rival_wool_context_v3_validation.json`.

Previous reference (21:49 UTC):
`runs/wool_family_context_v2_001/proposals/wool_family_context_v2/`;
`results/wool_family_context_v2_validation.json` retains its evidence.

''' + text[end:]
    readme.write_text(text)
    entry = f'''\n{now}: Promoted {CANDIDATE}. Public spending after six hires and one wheat purchase distinguishes the failed John sale from V5/2 seed purchases. Fresh57344 games: V5/2+6.8359pp and+$182.14 mean; all27other fullrecords unchanged. Native5120profiles,1024operational checks with active custom/native cases,302frozen C++inputs and256 rebuilt records pass. Packaging omission repaired without policy changes; amendment retained. Parent-self unchanged; no Git/upload.\n'''
    for filename in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md', 'OBJECTIVE_COVERAGE.md']:
        with (EXP / filename).open('a') as out:
            out.write(entry)
    print('Promoted', CANDIDATE, now)
    print('V5/2 native contrast', native['public_router_v52']['contrast'])


if __name__ == '__main__':
    main()
