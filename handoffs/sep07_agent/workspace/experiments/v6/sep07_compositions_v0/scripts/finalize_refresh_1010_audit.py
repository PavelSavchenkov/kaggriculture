"""Record measured notebook-port outcomes and precise component lineage."""
import hashlib
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
FRESH = EXP / 'research/refresh_1010/notebooks'
SCREEN = EXP / 'results/refresh_1010_screen'
OPPONENTS = ['shop_herd_s6_m3_g1', 'teammate_shoprouter', 'king_rc4', 'binghua_116', 'public_terminal_router']


def read(path):
    return json.loads(path.read_text())


def measure(name, opponent):
    result = read(SCREEN / f'{name}_vs_{opponent}.json')
    games = result['games']
    return {'games': len(games), 'wins': sum(g['cash'] > g['opponent_cash'] for g in games),
            'ties': sum(g['cash'] == g['opponent_cash'] for g in games),
            'mean_cash': sum(g['cash'] for g in games) / len(games),
            'mean_margin': sum(g['cash'] - g['opponent_cash'] for g in games) / len(games),
            'margin_cvar10': result['margin_cvar10'], 'pass_J': result['pass_J']}


def main():
    generic = read(SCREEN / 'titan_frontier_vs_market_impact_v4.json')
    debug = read(SCREEN / 'debug_pair16.json')
    assert generic['games'] == debug['games']
    ports = {}
    for name, parent in [('titan_frontier', 'kaito_v43'), ('market_impact_v4', 'public_router')]:
        comparisons = {}
        for rival in OPPONENTS:
            a, b = read(SCREEN / f'{name}_vs_{rival}.json'), read(SCREEN / f'{parent}_vs_{rival}.json')
            comparisons[rival] = {'new': measure(name, rival), 'parent': measure(parent, rival),
                                  'changed_complete_game_records': sum(x != y for x, y in zip(a['games'], b['games'])),
                                  'paired_mean_margin_delta': sum((x['cash'] - x['opponent_cash']) - (y['cash'] - y['opponent_cash']) for x, y in zip(a['games'], b['games'])) / len(a['games'])}
        directory = EXP / 'league' / name
        report = {'name': name, 'scenario': 'Fully enabled official rules with independent shop stream; discovery seeds1000..1063, both seats; no fresh promotion claim',
                  'source_parity': read(EXP / f'results/{name}_parity.json'),
                  'generic_debug_thread_full_record_matches': 16, 'self_play_games': 8,
                  'pass': measure(name, 'pass'), 'head_to_head_parent': measure(name, parent),
                  'common_opponents': comparisons,
                  'source_files': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((directory / 'source').glob('*'))},
                  'build': 'build/refresh_1010/generic/build.json',
                  'debug_build': 'build/refresh_1010/debug_pair/build.json',
                  'build_dependency_snapshot': 'build/refresh_1010/source_snapshot.json',
                  'status': 'Retained as a faithful audited external reference; no incumbent promotion'}
        if name == 'titan_frontier':
            report['learning'] = 'Similarity-gated early finished-product sales strongly beat the same production parent but are nearly inactive against this modern panel. Earlier sales also change funding and downstream production; do not call this an equal-production timing result.'
            report['causal_profile'] = 'results/refresh_1010_screen/titan_overlay_causal.json'
        else:
            report['learning'] = 'Literal projected-own-shed ranking under older price curves changes reachable actions, but worsens average margin versus every tested common opponent and loses to its Thomas parent.'
        (EXP / f'results/{name}_validation.json').write_text(json.dumps(report, indent=2) + '\n')
        ports[name] = report
        metadata = read(directory / 'IMPORT.json')
        metadata['status'] = f"{report['source_parity']['result']}; generic/debug/thread, PASS/self checks pass. Complete128-game common-seed panels retained as evidence; no global-strength or fresh-promotion claim."
        metadata['validation'] = f'results/{name}_validation.json'
        if name == 'titan_frontier':
            metadata['literal_curve_difference'] = 'Kaito sparse parent uses current official hinge curves for CARROT/TOMATO/EGG. Igor/LARK additional-sale ranking retains older log/linear/linear below-equilibrium curves.'
        (directory / 'IMPORT.json').write_text(json.dumps(metadata, indent=2) + '\n')
    titan = FRESH / 'tokenjunkielabs/titan-kaggriculture-frontier-source'
    metadata = read(EXP / 'league/titan_frontier/IMPORT.json')
    audit = {'source_sha256': metadata['source_sha256'], 'source_url': metadata['source_url'],
             'pinned_repository': metadata['pinned_repository'], 'authors': metadata['authors'],
             'license': metadata['license'], 'module_sha256': metadata['modules'], 'routes': metadata['routes'],
             'active_components': ['Three Kaito v43 fixed courses selected by first/second observed Yarn shop at88/153', 'All three bounded weed-repair controllers updated every turn; DIG, intended retry, eight prior-action replays', 'Kaito SELL-slot ordering by current official price-impact score and demand-recovery factor', 'Igor projected own shed and pickup reserves', 'LARK full finished-product sale quantities/additional SELL slots only when public farm signature distance<=2'],
             'inactive_components': metadata['inactive_source'], 'literal_curve_difference': metadata['literal_curve_difference'],
             'ports': ['league/titan_frontier', 'league/kaito_v43'], 'validation': 'results/titan_frontier_validation.json',
             'novelty_and_result': ports['titan_frontier']['learning'],
             'head_to_head_parent': ports['titan_frontier']['head_to_head_parent'],
             'best_agent_screen': ports['titan_frontier']['common_opponents']['shop_herd_s6_m3_g1'],
             'rating': None, 'decision': 'Keep the faithful port and baseline for league research; borrow the observed-similarity sale-regime idea, not an unverified blanket sale advance. No incumbent change.'}
    routes = read(titan / '_V43_ROUTES.json')
    audit['exact_raw_route_shared_prefix_lengths'] = {
        f'{a}/{b}': next((i for i, (x, y) in enumerate(zip(routes[a], routes[b])) if x != y), 719)
        for a in routes for b in routes if a < b}
    (titan / 'AUDIT.json').write_text(json.dumps(audit, indent=2) + '\n')
    market = FRESH / 'y3uanm/kaggriculture-market-impact-router-v4/AUDIT.json'
    audit = read(market)
    audit.update({'cpp_port': 'league/market_impact_v4', 'validation': 'results/market_impact_v4_validation.json', 'decision': ports['market_impact_v4']['learning'], 'head_to_head_parent': ports['market_impact_v4']['head_to_head_parent']})
    market.write_text(json.dumps(audit, indent=2) + '\n')
    parent = EXP / 'league/kaito_v43'
    (parent / 'IMPORT.json').write_text(json.dumps({'source': 'TITAN bundled unmodified Kaito v43 sparse shop policy; same source/module/route hashes as ../titan_frontier/IMPORT.json', 'parent_source_parity': read(EXP / 'results/kaito_v43_parity.json'), 'cpp_parent': 'titan_frontier::AgentCore(false)', 'purpose': 'Causal baseline; disables only the local sale overlay flag', 'license': 'Apache-2.0; retained license and notices in ../titan_frontier', 'rating': None}, indent=2) + '\n')
    (EXP / 'results/refresh_1010_notebook_audit.json').write_text(json.dumps({'ttv1': 'Exact byte duplicate of existing public_terminal_router; no second port', 'titan_frontier': 'results/titan_frontier_validation.json', 'market_impact_v4': 'results/market_impact_v4_validation.json', 'registration': 'No changes made to configs/league.json', 'submissions': 'None for this notebook audit task'}, indent=2) + '\n')
    print(json.dumps({name: report['head_to_head_parent'] for name, report in ports.items()}, indent=2))


if __name__ == '__main__':
    main()
