"""Summarize accepted descendants from their saved promotion evidence."""
from datetime import datetime, timezone
from pathlib import Path
import json

EXP = Path(__file__).resolve().parents[1]
SPECS = [
    ('crop_value_validation', [
        'Fertilize the late strawberry crop only when shops already seen justify it.',
        'Use a checked day-20 schedule; retain the existing animal and labor policy.',
        'Local service change on the inherited Justin replay course; activated games produce two extra strawberries.',
    ]),
    ('crop_rotation_berry_validation', [
        'Replace a three-tile wheat rotation with tomatoes when at least two tomato-consuming shops are visible on day 12.',
        'Borrow the tomato calendar from Mengfei Li replay 106429645, seat 0; rebuild placement, purchases, sales and 17 daily schedules locally.',
        'Preserve the earlier day-20 strawberry branch in both continuations; activated games exchange 36 wheat for 24 tomatoes.',
    ]),
    ('crop_mix_validation', [
        'Use productive wheat when the tomato branch is not selected or cannot enter.',
        'Borrow the productive wheat calendar from get some fries and combine it with the preceding tomato and strawberry branches.',
        'One fertilizer application and rebuilt schedules add six wheat per activated game.',
    ]),
    ('bohann_opening_validation', [
        'Borrow only the first two market turns from Bohann replay 106497007, seat 0.',
        'Keep our later farm and compiled schedules; the whole donor continuation did not help.',
        'Opening wheat transactions alter shared prices and rival liquidity; against King they also change later behavior.',
    ]),
    ('opening_market_validation', [
        'Search opening wheat transaction quantities and hiring while preserving the later farm.',
        'Choose the q32/b13 opening instead of the much larger Bohann transaction.',
        'Separate the parent-counter effect from the roughly $5 gains against most older opponents.',
    ]),
    ('late_portfolio_validation', [
        'On day 13, choose between retaining crops and adding a goose, cow or sheep on one eligible tile.',
        'Estimate complete remaining farm cash flows, compiled labor costs and shared-price effects from the public rival herd; average 32 possible future shop sequences conditional on shops already seen.',
        'Select expected margin minus half a sampled standard deviation, then execute checked schedules while preserving tomato and strawberry branches.',
    ]),
    ('wool_family_context_v2_validation', [
        'Import a whole sheep-heavy continuation from Thomas Tschinkel public V5/2.',
        'Choose it from observed early Yarn and milk-shop combinations; narrow the first rule after regressions against Junghoon.',
        'Restore entry stocks and use 17 locally compiled days, commonly saving 15–16 hires when those schedules activate.',
    ]),
    ('rival_wool_context_v3_validation', [
        'Retain accepted early sheep choices and allow additional choices after one more hour of public observations.',
        'Use actual rival hiring, net product flows and spending beyond hires and one wheat purchase to select the continuation.',
        'Exclude John’s failed-fertilizer-sale case, which fooled earlier rules; reuse the compiled sheep farm.',
    ]),
    ('observed_sale_lead_start216_validation', [
        'Borrow Ahmed V23’s earlier-sale, projected-shed and duplicate-suppression ideas; retain all inherited farm branches.',
        'Forecast the next sale request with an independent copy of our policy; advance eligible non-input sales only when projected own stock permits.',
        'Start at step 216, after initial farm funding, to avoid the demonstrated Junghoon regression from starting too early.',
    ]),
    ('empty_sale_slots_m2_population_validation', [
        'Remove zero-quantity non-input sale orders after step216 so later trades execute earlier within the same turn; preserve opening funding.',
        'Develop the transform locally after diagnosing Arlene V4 order-clamp behavior; inherit the accepted composition and worker policy.',
        'Select the unguarded version over its price-floor guard through a new independent population audit. Keep the old $6 per-game failures recorded rather than rewriting their verdict.',
    ]),
]

rows = []
for report, ideas in SPECS:
    path = EXP / 'results' / f'{report}.json'
    data = json.loads(path.read_text())
    candidate, parent = data['candidate'], data['parent']
    if 'paired_parent_evidence' in data:
        panel = data['paired_parent_evidence']
        metrics = panel['metrics']
        groups = panel['groups']['current']
        utilities, ci = groups['utilities'], groups['gain_95pct']
    elif 'confirmation' in data:
        panel = data['confirmation']
        metrics = panel['metrics']
        groups = panel['groups']['current']
        utilities, ci = groups['utilities'], groups['gain_95pct']
    elif 'metrics' in data['fresh']:
        panel = data['fresh']
        metrics = panel['metrics']
        groups = panel['groups']['current']
        utilities, ci = groups['utilities'], groups['gain_95pct']
    else:
        metrics = data['fresh']
        if 'grouped' in data:
            groups = data['grouped']['current']
            utilities, ci = groups['utilities'], groups['gain_95pct']
        else:
            utilities = data['group_objective']
            ci = data['seed_cluster_bootstrap_gain_95pct']
    direct = data.get('direct_extra', {}).get(parent, metrics[candidate][parent])
    if 'paired_parent_evidence' in data:
        direct = data['fresh']['metrics'][candidate][parent]
    n = direct['games']
    assert n == direct['wins'] + direct['ties'] + direct['losses']
    rows.append({
        'candidate': candidate, 'parent': parent, 'ideas': ideas,
        'report': str(path.relative_to(EXP)),
        'opponents': len(metrics[candidate]),
        'league_games_per_opponent': next(iter(metrics[candidate].values()))['games'],
        'parent_wtl': [direct['wins'], direct['ties'], direct['losses']],
        'parent_games': n,
        'parent_mean_margin': direct.get('mean_margin', direct.get('margin')),
        'league_parent_utility': utilities[parent],
        'league_candidate_utility': utilities[candidate],
        'league_gain_pp': 100 * (utilities[candidate] - utilities[parent]),
        'league_gain_95pct_pp': [100 * x for x in ci],
        'league_comparison_scope': data.get('paired_parent_scope', 'Paired parent control from this promotion panel.'),
    })

reference = json.loads((EXP / 'CURRENT_REFERENCE.json').read_text())
assert rows[-1]['candidate'] == reference['name']
assert rows[0]['parent'] == reference['last_submission']
assert all(row['parent'] == previous['candidate'] for previous, row in zip(rows, rows[1:]))
endpoint_report = json.loads((EXP / rows[-1]['report']).read_text())
endpoint_metrics = endpoint_report['fresh']['metrics'][reference['name']]
empty_report_path = EXP / 'results/empty_sale_slots_m2_validation.json'
empty_report = json.loads(empty_report_path.read_text())
floor_run = EXP / 'runs/empty_sale_floor_validation_sep08_001'
floor_report = json.loads((floor_run / 'FRESH_ANALYSIS.json').read_text())
floor_native = json.loads((floor_run / 'NATIVE_ANALYSIS.json').read_text())
assert not empty_report['promoted']
assert all(floor_report['gates'].values())
assert all(floor_native['gates'].values())
experimental = []
for data, candidate, direct, report, status in [
    (empty_report, empty_report['candidate'], empty_report['fresh_parent'],
     empty_report_path, 'Not promoted: two preregistered per-game checks each contain one $6 paired margin regression.'),
    (floor_report, floor_report['candidate'], floor_report['metrics'][floor_report['candidate']][floor_report['baseline']],
     floor_run / 'FRESH_ANALYSIS.json', 'All original audits pass; not selected after the independent comparison favors unguarded empty_sale_slots_m2.'),
]:
    if candidate in {r['candidate'] for r in rows}:
        continue
    groups = data['groups']['current']
    experimental.append({
        'candidate': candidate, 'parent': data['baseline'],
        'report': str(report.relative_to(EXP)), 'status': status,
        'parent_wtl': [direct['wins'], direct['ties'], direct['losses']],
        'parent_mean_margin': direct['mean_margin'],
        'league_parent_utility': groups['utilities'][data['baseline']],
        'league_candidate_utility': groups['utilities'][candidate],
    })
snapshot = {
    'created_utc': datetime.now(timezone.utc).isoformat(),
    'last_submission_id': 56078898,
    'last_submitted_agent': 'investment_context_guarded_001_best',
    'current_reference': reference,
    'accepted_descendants': rows,
    'experimental_descendants': experimental,
    'endpoint_metrics': endpoint_metrics,
    'scope': 'Saved local C++ validation; no current Kaggle rank inference. League utility is win=1, tie=0.5, loss=0, averaged equally across defined opponent groups. Seeds and league membership change between promotions; compare each row with its own paired parent control.',
}
early_path = EXP / 'runs/early_melon_sep08_001/FRESH_ANALYSIS.json'
early = json.loads(early_path.read_text())
selected_cold = next(row for row in early['results'] if row['candidate'] == early['selected'])
assert selected_cold['eligible'] and all(selected_cold['gates'].values())
snapshot['separate_composition_branch'] = {
    'report': str(early_path.relative_to(EXP)),
    'selected': early['selected'],
    'metrics': selected_cold,
    'scope': 'Separate cold-farm selection; not a successor to the strongest mainline agent.',
}
(EXP / 'research/post_submission_lineage.json').write_text(json.dumps(snapshot, indent=2) + '\n')
lines = [
    '# Agent lineage from the last submitted version', '',
    f"Snapshot: {snapshot['created_utc']}. Last submitted: investment_context_guarded_001_best, submission 56078898. Current accepted local reference: {reference['name']}.", '',
    f'{len(rows)} accepted descendants follow. Each row inherits the preceding row, starting from the submitted agent. These are local C++ results, not Kaggle rankings.', '',
    'W/T/L means wins/ties/losses in direct parent matches, both seats. League utility counts a tie as half a win and averages the defined opponent groups equally. Each arrow compares candidate and parent on the same seeds and opponents. League membership and seeds change between rows, so the arrows are not a single cumulative benchmark.', '',
    '| Agent | Direct parent W/T/L | Mean parent margin | Panel opponents | Paired league utility | Gain, percentage points |',
    '|---|---:|---:|---:|---:|---:|',
]
for row in rows:
    wtl = '/'.join(f'{v:,}' for v in row['parent_wtl'])
    lines.append(f"| [{row['candidate']}](../{row['report']}) | {wtl} | ${row['parent_mean_margin']:+,.2f} | {row['opponents']} | {row['league_parent_utility']:.3%} → {row['league_candidate_utility']:.3%} | {row['league_gain_pp']:+.3f} |")
lines += ['', 'League matches use 1,024 games per opponent except late_value_s32_t0_r05, whose independent confirmation uses 4,096. The crop-mix parent result uses its additional 4,096-game direct audit.', '']
for i, row in enumerate(rows, 1):
    lines += [f"## {i}. {row['candidate']}", '', f"Parent: {row['parent']}.", '']
    lines += [f'- {idea}' for idea in row['ideas']]
    low, high = row['league_gain_95pct_pp']
    lines += ['', f'Paired league gain 95% bootstrap interval: {low:+.3f} to {high:+.3f} percentage points.', '']
    if row['candidate'] == 'empty_sale_slots_m2':
        lines += [row['league_comparison_scope'], '',
            'The direct parent result in the table uses the new 2200000 panel. The paired parent league arrow uses the original 2000000 panel. New independent selection versus the guard: 71,680 fresh games and 6,144 native/PASS games; all population gates pass, with unchanged operational/frozen evidence. No old failed gate is relabeled as passed.', '']
lines += [
    '## Tradeoffs and branches not accepted', '',
    '- Bohann: King gains 18.164 percentage points and $16,707.94 mean margin, while public V5 loses 2/1,024 wins and $9.28 mean margin.',
    '- q32: its 1,020/1,024 parent wins are a large direct counter. The historical league is almost unchanged (94.688% → 94.674%); two fresh opponents lose one win each.',
    '- Late portfolio: positive broad confirmation, but some older crop agents lose a small number of wins. It handles one investment slot and date, not arbitrary whole-farm composition search.',
    '- Wool v2: V5/2 gains 6.641 percentage points; teammate loses 1/1,024 wins despite a $62.10 mean-margin gain. The unaccepted wool v1 specialist still beats v2 directly: v2 has 80 wins, 816 ties, 128 losses.',
    '- Rival wool v3: all improvement is against V5/2 (+6.836 percentage points, +$182.14 mean margin). All 27 other complete opponent result sets equal the parent. Its 86/852/86 parent match exactly equals parent self-play; there is no direct improvement.',
    '- Rival wool v1 and v2 were rejected after false branch activations against John. V3 adds observed spending to resolve those demonstrated cases.',
    '- Wool purchase/wheat repair v2: 245,760 fresh games; V5/2 gains $167.44 mean margin but only 3/4,096 additional wins. The required positive lower confidence bound for league utility was not met, so it is not promoted.',
    '- Ahmed V23 is a separately verified public C++ port, not an accepted descendant. It loses 183/256 discovery games against rival_wool_context_v3. Layer ablations identify earlier sales and storage protection as useful components.',
    '- Earlier sale-lead variants were not promoted: the broad discovery gain hid a Junghoon regression. The accepted step-216 version protects initial funding and passes 65,536 fresh games across candidate and parent, 32 opponents, 5,632 native/PASS profiles, 1,024 operational games and frozen-source rebuilding. Its historical-group utility also improves, 95.058% to 95.747%.', '',
    '## Current endpoint on its latest fresh panel', '',
]
for opponent, label in [
    (reference['previous'], 'immediate parent'),
    (reference['last_submission'], 'last submitted agent'),
    ('teammate_shoprouter', 'teammate'),
    ('public_router_v52', 'public V5/2'),
    ('ahmed_v23', 'Ahmed V23'),
]:
    metric = endpoint_metrics[opponent]
    lines.append(f"- Against {label}: {metric['wins']:,}/{metric['games']:,} wins ({metric['wins'] / metric['games']:.3%}), {metric['ties']} ties, mean margin ${metric['mean_margin']:+,.2f}.")
lines += [
    '- Accepted reference, committed catalog agent (bohann_opening_v1), and submitted agent are different frozen versions.', '',
    'Reproduction and exact lineage: linked promotion reports, ../LINEAGE.md, each referenced run’s source/IMPORT/LINEAGE files, and ../CURRENT_REFERENCE.json. General raw-composition valuation, new placement and cold construction remain incomplete.', '',
]
lines += ['## Experimental branch not selected', '',
          'The floor guard starts from observed_sale_lead_start_216. The unguarded sibling is now accepted following the independent population comparison.', '',
          '| Agent | Direct accepted-parent W/T/L | Mean parent margin | Paired league utility | Status |',
          '|---|---:|---:|---:|---|']
for row in experimental:
    wtl = '/'.join(f'{v:,}' for v in row['parent_wtl'])
    lines.append(f"| [{row['candidate']}](../{row['report']}) | {wtl} | ${row['parent_mean_margin']:+,.2f} | {row['league_parent_utility']:.3%} → {row['league_candidate_utility']:.3%} | {row['status']} |")
duel = floor_report['metrics']['empty_sale_floor_m1']['empty_sale_slots_m2']
lines += ['',
    '- empty_sale_slots_m2: remove zero-quantity non-input sale orders after initial funding so later orders execute earlier within the turn. This is a local transform motivated by exact diagnosis of Arlene V4’s order clamp; no external policy block was copied.',
    '- empty_sale_floor_m1: preserve an empty slot when advancing later own sales can reach the $1 price floor. The guard fixes the two demonstrated failures, but gives up some transaction benefit.',
    f"- Original guard-versus-unguarded panel: {duel['wins']} wins, {duel['ties']} ties, {duel['losses']} losses in {duel['games']:,} direct games; mean margin ${duel['mean_margin']:+,.2f}. The later independent population comparison selects the unguarded sibling; passing a stricter rule alone did not establish greater playing strength.",
    '- The unguarded audit has 67,584 fresh games across 33 opponents; the guarded audit has 69,632 across 34. Each also has 5,632 native/PASS games, 1,024 operational games and 64 isolated rebuilt games. Their league percentages use different panels and must not be compared directly.', '',
]
lines += [
    '## Separate composition-search branch', '',
    'These agents explore farms built from dated crop and animal lifetimes. They do not inherit the strongest mainline policy. Their smaller opponent panels are not comparable with the mainline league percentages.', '',
    '- joint_routes_p362_m0 is the unchanged original compiler control for the larger mixed p362 farm. Joint-route replacements were rejected: extra animal service displaced crop work and hurt dense farms.',
    '- day_program_p362_m3 reuses locally solved day-14/15/16 schedules after checking current observations. Discovery improved margin against the public router and main reference by $719.50 and $519.25, but lost $527 against the original compiler. This was an experimental stepping stone, not a broad promotion.',
    '- service_bank_p362_m2 adds a day-16 program that completes five previously missed feeds with two fewer workers. In 2,560 fresh games it passed the cold-branch gates: 182 wins, 10 ties, 64 losses against day_program_p362_m3, mean margin +$1,185.63; equal-opponent own-cash gain +$614.41. It still lost every game against the main reference and public router. See ../runs/day_service_bank_sep08_001/FRESH_RESULTS.md.',
    '- cold_renewal_p98 replaces four geese with cows and renews empty tiles with tomatoes and carrots; p157 omits the geese and renews wheat with a smaller workforce. Both failed the 5,376-game fresh selection. Against service_bank, p98 had 138/1/117 W/T/L and +$308.26 margin, but active-field utility fell 2.051 percentage points; p157 raised own cash $3,123.89 while lowering match margin $803.12. See ../runs/cold_renewal_sep08_001/FRESH_RESULTS.md.',
    '- early_melon_b98_m1 inherits the p98 cow-heavy composition and harvests melons at age 10 instead of waiting about two more days, preserving later planting dates. The calendar comes from top-player replay observations, including get some fries episode 106686702, seat 0. The m2 sibling also regenerates following crop dates. Both pass the separate cold-branch gates; m1 wins the declared selection by slightly higher active-field utility. This does not establish a significant m1-versus-m2 advantage.', '',
    '| Selected early-melon agent versus | W/T/L | Mean margin |',
    '|---|---:|---:|',
]
for opponent in ['cold_renewal_p98', 'service_bank_p362_m2', 'empty_sale_slots_m2', 'teammate_shoprouter']:
    row = next(row for row in selected_cold['rows'] if row['opponent'] == opponent)
    lines.append(f"| {opponent} | {'/'.join(str(v) for v in row['wtl'])} | ${row['mean_margin']:+,.2f} |")
lines += ['',
    f"Across seven equally weighted active opponents, selected early-melon utility improves {100 * selected_cold['active_mean_gain'][0]:.3f} percentage points over service_bank, with mean margin gain ${selected_cold['active_mean_gain'][1]:,.2f}. All gained wins are against the weaker cold-farm agents; it still wins zero against each of the four strong opponents. The complete audit contains {early['games']:,} games. See ../runs/early_melon_sep08_001/FRESH_RESULTS.md and SELECTION.json.", '',
    'shop_branch_m2 now implements the fitted day-6 rule: choose the old cow-renewal farm when neither of the first two observed shops uses yarn or eggs. Its 1,632 common prefixes and complete selected constituent outcomes are verified; 224 operational games pass. The 7,776-game fresh audit gains 2.637 utility percentage points and $2,396.36 mean margin over service_bank, with positive confidence bounds. It loses 229/256 games against early_melon_b98_m1 and is not selected as the current cold reference. See ../runs/shop_branch_sep08_001/FRESH_RESULTS.md and SELECTION.json.', '',
    'Mainline crop_context_m0/m1/m2 isolate the strongest agent and preserve all later improvements while keeping the current crop gate, preferring wheat, or requesting tomatoes. In 4,608 discovery games, both forced choices lose value. All 1,152 control records equal the original agent, 1,152 common prefixes are exact, and 52 operational games pass. Even hindsight selection among these choices has only $14.08 mean-margin headroom on eight active opponents. Neither forced agent is promoted; no new price-selector fit is prioritized for this narrow pair. See ../runs/crop_context_sep08_001/RESULTS.md and SELECTION.json.', '',
]
(EXP / 'research/post_submission_lineage.md').write_text('\n'.join(lines))
for row in rows:
    print(row['candidate'], row['parent_wtl'], round(row['league_gain_pp'], 4))
