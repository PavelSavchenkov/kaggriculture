# Agent lineage from the last submitted version

Snapshot: 2026-09-08T09:19:02.160272+00:00. Last submitted: investment_context_guarded_001_best, submission 56078898. Current accepted local reference: empty_sale_slots_m2.

10 accepted descendants follow. Each row inherits the preceding row, starting from the submitted agent. These are local C++ results, not Kaggle rankings.

W/T/L means wins/ties/losses in direct parent matches, both seats. League utility counts a tie as half a win and averages the defined opponent groups equally. Each arrow compares candidate and parent on the same seeds and opponents. League membership and seeds change between rows, so the arrows are not a single cumulative benchmark.

| Agent | Direct parent W/T/L | Mean parent margin | Panel opponents | Paired league utility | Gain, percentage points |
|---|---:|---:|---:|---:|---:|
| [crop_value_m2_t4](../results/crop_value_validation.json) | 278/664/82 | $+62.23 | 15 | 88.395% → 90.240% | +1.845 |
| [crop_rotation_t2_berry](../results/crop_rotation_berry_validation.json) | 199/758/67 | $+150.93 | 17 | 90.277% → 91.294% | +1.017 |
| [crop_mix_t2_wheat](../results/crop_mix_validation.json) | 1,801/2,010/285 | $+67.45 | 18 | 91.079% → 93.197% | +2.118 |
| [bohann_opening_v1](../results/bohann_opening_validation.json) | 969/0/55 | $+34.76 | 19 | 93.587% → 94.999% | +1.412 |
| [opening_q32_b13_v1](../results/opening_market_validation.json) | 1,020/0/4 | $+8,415.24 | 20 | 92.919% → 94.150% | +1.232 |
| [late_value_s32_t0_r05](../results/late_portfolio_validation.json) | 1,527/2,256/313 | $+173.98 | 22 | 94.513% → 94.680% | +0.167 |
| [wool_family_context_v2](../results/wool_family_context_v2_validation.json) | 139/818/67 | $+363.36 | 25 | 93.927% → 94.356% | +0.429 |
| [rival_wool_context_v3](../results/rival_wool_context_v3_validation.json) | 86/852/86 | $+0.00 | 28 | 94.190% → 94.475% | +0.285 |
| [observed_sale_lead_start_216](../results/observed_sale_lead_start216_validation.json) | 959/0/65 | $+1,587.89 | 32 | 90.710% → 94.697% | +3.986 |
| [empty_sale_slots_m2](../results/empty_sale_slots_m2_population_validation.json) | 911/40/73 | $+242.12 | 33 | 93.956% → 94.725% | +0.769 |

League matches use 1,024 games per opponent except late_value_s32_t0_r05, whose independent confirmation uses 4,096. The crop-mix parent result uses its additional 4,096-game direct audit.

## 1. crop_value_m2_t4

Parent: investment_context_guarded_001_best.

- Fertilize the late strawberry crop only when shops already seen justify it.
- Use a checked day-20 schedule; retain the existing animal and labor policy.
- Local service change on the inherited Justin replay course; activated games produce two extra strawberries.

Paired league gain 95% bootstrap interval: +1.459 to +2.256 percentage points.

## 2. crop_rotation_t2_berry

Parent: crop_value_m2_t4.

- Replace a three-tile wheat rotation with tomatoes when at least two tomato-consuming shops are visible on day 12.
- Borrow the tomato calendar from Mengfei Li replay 106429645, seat 0; rebuild placement, purchases, sales and 17 daily schedules locally.
- Preserve the earlier day-20 strawberry branch in both continuations; activated games exchange 36 wheat for 24 tomatoes.

Paired league gain 95% bootstrap interval: +0.712 to +1.359 percentage points.

## 3. crop_mix_t2_wheat

Parent: crop_rotation_t2_berry.

- Use productive wheat when the tomato branch is not selected or cannot enter.
- Borrow the productive wheat calendar from get some fries and combine it with the preceding tomato and strawberry branches.
- One fertilizer application and rebuilt schedules add six wheat per activated game.

Paired league gain 95% bootstrap interval: +1.784 to +2.468 percentage points.

## 4. bohann_opening_v1

Parent: crop_mix_t2_wheat.

- Borrow only the first two market turns from Bohann replay 106497007, seat 0.
- Keep our later farm and compiled schedules; the whole donor continuation did not help.
- Opening wheat transactions alter shared prices and rival liquidity; against King they also change later behavior.

Paired league gain 95% bootstrap interval: +1.170 to +1.666 percentage points.

## 5. opening_q32_b13_v1

Parent: bohann_opening_v1.

- Search opening wheat transaction quantities and hiring while preserving the later farm.
- Choose the q32/b13 opening instead of the much larger Bohann transaction.
- Separate the parent-counter effect from the roughly $5 gains against most older opponents.

Paired league gain 95% bootstrap interval: +1.213 to +1.245 percentage points.

## 6. late_value_s32_t0_r05

Parent: opening_q32_b13_v1.

- On day 13, choose between retaining crops and adding a goose, cow or sheep on one eligible tile.
- Estimate complete remaining farm cash flows, compiled labor costs and shared-price effects from the public rival herd; average 32 possible future shop sequences conditional on shops already seen.
- Select expected margin minus half a sampled standard deviation, then execute checked schedules while preserving tomato and strawberry branches.

Paired league gain 95% bootstrap interval: +0.074 to +0.258 percentage points.

## 7. wool_family_context_v2

Parent: late_value_s32_t0_r05.

- Import a whole sheep-heavy continuation from Thomas Tschinkel public V5/2.
- Choose it from observed early Yarn and milk-shop combinations; narrow the first rule after regressions against Junghoon.
- Restore entry stocks and use 17 locally compiled days, commonly saving 15–16 hires when those schedules activate.

Paired league gain 95% bootstrap interval: +0.203 to +0.669 percentage points.

## 8. rival_wool_context_v3

Parent: wool_family_context_v2.

- Retain accepted early sheep choices and allow additional choices after one more hour of public observations.
- Use actual rival hiring, net product flows and spending beyond hires and one wheat purchase to select the continuation.
- Exclude John’s failed-fertilizer-sale case, which fooled earlier rules; reuse the compiled sheep farm.

Paired league gain 95% bootstrap interval: +0.195 to +0.382 percentage points.

## 9. observed_sale_lead_start_216

Parent: rival_wool_context_v3.

- Borrow Ahmed V23’s earlier-sale, projected-shed and duplicate-suppression ideas; retain all inherited farm branches.
- Forecast the next sale request with an independent copy of our policy; advance eligible non-input sales only when projected own stock permits.
- Start at step 216, after initial farm funding, to avoid the demonstrated Junghoon regression from starting too early.

Paired league gain 95% bootstrap interval: +3.739 to +4.243 percentage points.

## 10. empty_sale_slots_m2

Parent: observed_sale_lead_start_216.

- Remove zero-quantity non-input sale orders after step216 so later trades execute earlier within the same turn; preserve opening funding.
- Develop the transform locally after diagnosing Arlene V4 order-clamp behavior; inherit the accepted composition and worker policy.
- Select the unguarded version over its price-floor guard through a new independent population audit. Keep the old $6 per-game failures recorded rather than rewriting their verdict.

Paired league gain 95% bootstrap interval: +0.709 to +0.835 percentage points.

The original 2000000 paired candidate/accepted-parent panel supplies the parent league comparison. It failed the old every-game no-regression rule; that verdict is unchanged. The independent 2200000/2204000 comparison supplies the new population selection evidence against the guarded challenger.

The direct parent result in the table uses the new 2200000 panel. The paired parent league arrow uses the original 2000000 panel. New independent selection versus the guard: 71,680 fresh games and 6,144 native/PASS games; all population gates pass, with unchanged operational/frozen evidence. No old failed gate is relabeled as passed.

## Tradeoffs and branches not accepted

- Bohann: King gains 18.164 percentage points and $16,707.94 mean margin, while public V5 loses 2/1,024 wins and $9.28 mean margin.
- q32: its 1,020/1,024 parent wins are a large direct counter. The historical league is almost unchanged (94.688% → 94.674%); two fresh opponents lose one win each.
- Late portfolio: positive broad confirmation, but some older crop agents lose a small number of wins. It handles one investment slot and date, not arbitrary whole-farm composition search.
- Wool v2: V5/2 gains 6.641 percentage points; teammate loses 1/1,024 wins despite a $62.10 mean-margin gain. The unaccepted wool v1 specialist still beats v2 directly: v2 has 80 wins, 816 ties, 128 losses.
- Rival wool v3: all improvement is against V5/2 (+6.836 percentage points, +$182.14 mean margin). All 27 other complete opponent result sets equal the parent. Its 86/852/86 parent match exactly equals parent self-play; there is no direct improvement.
- Rival wool v1 and v2 were rejected after false branch activations against John. V3 adds observed spending to resolve those demonstrated cases.
- Wool purchase/wheat repair v2: 245,760 fresh games; V5/2 gains $167.44 mean margin but only 3/4,096 additional wins. The required positive lower confidence bound for league utility was not met, so it is not promoted.
- Ahmed V23 is a separately verified public C++ port, not an accepted descendant. It loses 183/256 discovery games against rival_wool_context_v3. Layer ablations identify earlier sales and storage protection as useful components.
- Earlier sale-lead variants were not promoted: the broad discovery gain hid a Junghoon regression. The accepted step-216 version protects initial funding and passes 65,536 fresh games across candidate and parent, 32 opponents, 5,632 native/PASS profiles, 1,024 operational games and frozen-source rebuilding. Its historical-group utility also improves, 95.058% to 95.747%.

## Current endpoint on its latest fresh panel

- Against immediate parent: 911/1,024 wins (88.965%), 40 ties, mean margin $+242.12.
- Against last submitted agent: 967/1,024 wins (94.434%), 0 ties, mean margin $+2,548.34.
- Against teammate: 1,009/1,024 wins (98.535%), 0 ties, mean margin $+11,933.89.
- Against public V5/2: 927/1,024 wins (90.527%), 0 ties, mean margin $+3,461.41.
- Against Ahmed V23: 876/1,024 wins (85.547%), 0 ties, mean margin $+2,943.78.
- Accepted reference, committed catalog agent (bohann_opening_v1), and submitted agent are different frozen versions.

Reproduction and exact lineage: linked promotion reports, ../LINEAGE.md, each referenced run’s source/IMPORT/LINEAGE files, and ../CURRENT_REFERENCE.json. General raw-composition valuation, new placement and cold construction remain incomplete.

## Experimental branch not selected

The floor guard starts from observed_sale_lead_start_216. The unguarded sibling is now accepted following the independent population comparison.

| Agent | Direct accepted-parent W/T/L | Mean parent margin | Paired league utility | Status |
|---|---:|---:|---:|---|
| [empty_sale_floor_m1](../runs/empty_sale_floor_validation_sep08_001/FRESH_ANALYSIS.json) | 901/48/75 | $+222.76 | 92.504% → 93.463% | All original audits pass; not selected after the independent comparison favors unguarded empty_sale_slots_m2. |

- empty_sale_slots_m2: remove zero-quantity non-input sale orders after initial funding so later orders execute earlier within the turn. This is a local transform motivated by exact diagnosis of Arlene V4’s order clamp; no external policy block was copied.
- empty_sale_floor_m1: preserve an empty slot when advancing later own sales can reach the $1 price floor. The guard fixes the two demonstrated failures, but gives up some transaction benefit.
- Original guard-versus-unguarded panel: 76 wins, 446 ties, 502 losses in 1,024 direct games; mean margin $-12.51. The later independent population comparison selects the unguarded sibling; passing a stricter rule alone did not establish greater playing strength.
- The unguarded audit has 67,584 fresh games across 33 opponents; the guarded audit has 69,632 across 34. Each also has 5,632 native/PASS games, 1,024 operational games and 64 isolated rebuilt games. Their league percentages use different panels and must not be compared directly.

## Separate composition-search branch

These agents explore farms built from dated crop and animal lifetimes. They do not inherit the strongest mainline policy. Their smaller opponent panels are not comparable with the mainline league percentages.

- joint_routes_p362_m0 is the unchanged original compiler control for the larger mixed p362 farm. Joint-route replacements were rejected: extra animal service displaced crop work and hurt dense farms.
- day_program_p362_m3 reuses locally solved day-14/15/16 schedules after checking current observations. Discovery improved margin against the public router and main reference by $719.50 and $519.25, but lost $527 against the original compiler. This was an experimental stepping stone, not a broad promotion.
- service_bank_p362_m2 adds a day-16 program that completes five previously missed feeds with two fewer workers. In 2,560 fresh games it passed the cold-branch gates: 182 wins, 10 ties, 64 losses against day_program_p362_m3, mean margin +$1,185.63; equal-opponent own-cash gain +$614.41. It still lost every game against the main reference and public router. See ../runs/day_service_bank_sep08_001/FRESH_RESULTS.md.
- cold_renewal_p98 replaces four geese with cows and renews empty tiles with tomatoes and carrots; p157 omits the geese and renews wheat with a smaller workforce. Both failed the 5,376-game fresh selection. Against service_bank, p98 had 138/1/117 W/T/L and +$308.26 margin, but active-field utility fell 2.051 percentage points; p157 raised own cash $3,123.89 while lowering match margin $803.12. See ../runs/cold_renewal_sep08_001/FRESH_RESULTS.md.
- early_melon_b98_m1 inherits the p98 cow-heavy composition and harvests melons at age 10 instead of waiting about two more days, preserving later planting dates. The calendar comes from top-player replay observations, including get some fries episode 106686702, seat 0. The m2 sibling also regenerates following crop dates. Both pass the separate cold-branch gates; m1 wins the declared selection by slightly higher active-field utility. This does not establish a significant m1-versus-m2 advantage.

| Selected early-melon agent versus | W/T/L | Mean margin |
|---|---:|---:|
| cold_renewal_p98 | 252/0/4 | $+7,311.07 |
| service_bank_p362_m2 | 222/0/34 | $+8,382.08 |
| empty_sale_slots_m2 | 0/0/256 | $-53,792.83 |
| teammate_shoprouter | 0/0/256 | $-47,181.70 |

Across seven equally weighted active opponents, selected early-melon utility improves 11.970 percentage points over service_bank, with mean margin gain $5,687.63. All gained wins are against the weaker cold-farm agents; it still wins zero against each of the four strong opponents. The complete audit contains 8,192 games. See ../runs/early_melon_sep08_001/FRESH_RESULTS.md and SELECTION.json.

shop_branch_m2 now implements the fitted day-6 rule: choose the old cow-renewal farm when neither of the first two observed shops uses yarn or eggs. Its 1,632 common prefixes and complete selected constituent outcomes are verified; 224 operational games pass. The 7,776-game fresh audit gains 2.637 utility percentage points and $2,396.36 mean margin over service_bank, with positive confidence bounds. It loses 229/256 games against early_melon_b98_m1 and is not selected as the current cold reference. See ../runs/shop_branch_sep08_001/FRESH_RESULTS.md and SELECTION.json.

Mainline crop_context_m0/m1/m2 isolate the strongest agent and preserve all later improvements while keeping the current crop gate, preferring wheat, or requesting tomatoes. In 4,608 discovery games, both forced choices lose value. All 1,152 control records equal the original agent, 1,152 common prefixes are exact, and 52 operational games pass. Even hindsight selection among these choices has only $14.08 mean-margin headroom on eight active opponents. Neither forced agent is promoted; no new price-selector fit is prioritized for this narrow pair. See ../runs/crop_context_sep08_001/RESULTS.md and SELECTION.json.
