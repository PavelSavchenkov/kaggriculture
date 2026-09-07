"""Freeze measured readiness and distinguish claims from observed behavior."""
import hashlib
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
PACKAGE = EXP / 'league/public_router_v5'
SCREEN = EXP / 'results/refresh_1112_screen'
SOURCE = EXP / 'research/refresh_1112/notebooks/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router'
RIVALS = ['shop_herd_guarded_001_best', 'teammate_shoprouter', 'king_rc4', 'binghua_116', 'public_terminal_router']


def read(path):
    return json.loads(path.read_text())


def measure(name):
    data = read(SCREEN / f'{name}.json')
    games = data['games']
    return {'games': len(games), 'wins': sum(x['cash'] > x['opponent_cash'] for x in games),
            'ties': sum(x['cash'] == x['opponent_cash'] for x in games),
            'mean_cash': sum(x['cash'] for x in games) / len(games),
            'mean_margin': sum(x['cash'] - x['opponent_cash'] for x in games) / len(games),
            'mean_failed_unit_actions': sum(x['unit_faults'] for x in games) / len(games),
            'margin_cvar10': data['margin_cvar10'], 'pass_J': data['pass_J'],
            'all_complete_719_turns': all(x['turns'] == 719 for x in games),
            'scenario': data['scenario']}


def main():
    metadata = read(PACKAGE / 'IMPORT.json')
    comparison = {}
    for rival in RIVALS:
        a = measure(f'public_router_v5_vs_{rival}')
        b = measure(f'public_sixday_vs_{rival}')
        comparison[rival] = {'public_router_v5': a, 'older_public_sixday': b,
                             'paired_mean_margin_difference': a['mean_margin'] - b['mean_margin']}
    commands = read(SCREEN / 'COMMANDS.json')
    source_hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((PACKAGE / 'source').iterdir())}
    builds = {}
    for mode in ['generic', 'debug_pair']:
        directory = EXP / 'build/refresh_1112' / mode
        build = read(directory / 'build.json')
        for p, sha in source_hashes.items():
            assert build['source_sha256'][p] == sha
        builds[mode] = {'build_manifest': str((directory / 'build.json').relative_to(EXP)),
                        'frozen_dependency_tree': str((directory / 'source_snapshot').relative_to(EXP)),
                        'binary_sha256': hashlib.sha256((directory / 'arena').read_bytes()).hexdigest(),
                        'current_policy_matches_frozen_compiled_sources': True}
    operational_differences = [
        'Source-author93.76% against689 frozen tapes is not an independently verified current-league win rate; supplied notebook omits the full evaluation outcomes and provenance.json',
        'All five full tapes have short differing prefixes (first divergence1–12), rather than literally identical full opening actions; safe physical compatibility at every splice is not proven by the notebook text',
        'The actual callable unconditionally selects tape0 again at288 and432; it does not retain the previously selected tape across those boundaries',
        'Only Yarn count, observed milk demand, and carrot quote are read by decision nodes; opponent/cash/tile features computed by Python do not influence this particular controller',
        'Current local matches have roughly45–46 failed unit actions per game. This does not establish tile desynchronization; it also does not support a blanket claim that every scheduled action succeeds',
        'Local C++ normalizes active worker count and semantically ignored action arguments; reset, backwards step, and719-or-later PASS behavior match the supplied callable'
    ]
    report = {'package': 'league/public_router_v5', 'source_sha256': metadata['source_sha256'],
              'source_parity': read(EXP / 'results/public_router_v5_parity.json'),
              'common_seed_screen': {'seeds': '1000..1063, both seats', 'kind': 'Discovery panel, not a fresh promotion gate', 'comparisons': comparison},
              'head_to_head_older_public_sixday': measure('public_router_v5_vs_public_sixday'),
              'pass': measure('pass256'), 'self': measure('self8'),
              'native_shop_incumbent_check': measure('native_best128'),
              'generic_debug_thread_full_records_identical': commands['generic_debug_thread_records_identical'],
              'builds': builds, 'policy_source_sha256': source_hashes,
              'operational_and_source_claim_differences': operational_differences,
              'decision': 'Faithful distinct strong external opponent ready for catalog registration; still below our guarded adaptive incumbent on this panel; no submission or promotion',
              'source_hashes_of_results': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(SCREEN.glob('*.json'))}}
    (EXP / 'results/public_router_v5_validation.json').write_text(json.dumps(report, indent=2) + '\n')
    metadata['validation'] = 'results/public_router_v5_validation.json'
    metadata['status'] = report['decision']
    metadata['policy_source_sha256'] = source_hashes
    metadata['operational_and_source_claim_differences'] = operational_differences
    (PACKAGE / 'IMPORT.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (SOURCE / 'AUDIT.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (PACKAGE / 'README.md').write_text('''Public router v5 — Thomas93.8 notebook

Faithful C++ port of the downloaded Thomas Tschinkel five-tape controller. At144 it selects a Yarn, milk-demand or balanced continuation; at288/432 it selects base tape0; at576 it selects by carrot quote54. Original source, exact hashes, measured nearest public-course matches and attribution limits are in IMPORT.json. No original episode mapping or separate code license was supplied by the notebook.

10,073 original-source action comparisons pass, including every route and reset/rewind/end cases. Frozen generic and debug builds match all16 game records across thread counts. PASS256 and self8 complete; all self games tie.

Discovery seeds1000–1063,both seats:36/128 wins against our guarded adaptive incumbent (mean margin−2777),128/128 teammate (+25422),85/128 King (+1997),84/128 Binghua (+1338),128/128 public terminal (+13893). The independent native-shop check is44/128 against the incumbent (−2477). These are small local panels, not the notebook-reported93.76% frozen-ladder result.

Retain as a strong external opponent and strategy source. No incumbent promotion or submission. Full records and commands: results/refresh_1112_screen/. Validation: results/public_router_v5_validation.json. Builders and source-parity oracle: scripts/build_router_v5.py and scripts/verify_router_v5.py. No configs were changed by this audit.
''')
    print(json.dumps({'status': report['decision'], 'policy_sha256': source_hashes, 'best': comparison[RIVALS[0]]['public_router_v5']}, indent=2))


if __name__ == '__main__':
    main()
