# League composition, evaluation and promotion

This document preserves the exact last broad protocol. For a future experiment,
freeze its league and gates before new results. Do not silently reuse these
exposed seeds as unseen confirmation.

## Most recent completed broad test

Three agents: empty_sale_slots_m2, animal_repair_premium_m2 and
animal_repair_q24_premium_m2. There are40 opponents in the independent-shop panel
and13 in the native panel. Each independent matchup has512 seeds ×2 seats from
2360000; native has128 seeds ×2 seats from2364000. Total132,864 matches.

The independent shop stream is sampled by the evaluator and revealed only when
the engine unlocks a shop. It prevents policy-dependent weed RNG consumption
from changing the paired shop sequence. Native games retain official engine
RNG coupling. They answer different questions and are reported separately.

Each primary opponent receives equal weight; historical opponents form a
separate equally weighted group. Ties count0.5. Bootstrap resamples entire seed
clusters with both seats paired and opponents jointly, using4,000 samples.
Worst-decile cash/margin is computed on each distribution, not just selected
wins. Full per-opponent results and guard diagnostics are retained.

Primary8: teammate_shoprouter, public_router, public_router_v52, ahmed_v23, junghoon_wool_sales, king_rc4, yusuke_sep08_m2, ahmed_v24.

Historical31: animal_adaptive_r1_c0_b0, binghua_116, bohann_opening_v1, crop_mix_t2_wheat, crop_rotation_t2_berry, crop_value_m2_t4, empty_sale_floor_m1, investment_context_guarded_001_best, john_131, late_goose_wheat_context, late_value_s32_t0_r05, observed_sale_lead_start_216, opening_q32_b13_v1, opening_router_v4, public_capacity_router, public_router_v5, public_sixday, rival_wool_context_v1, rival_wool_context_v2, rival_wool_context_v3, rival_wool_purchase_repair_v1, shop_herd_guarded_001_best, shop_herd_s6_m3_g1, ticket_p116_t9_i9, wheat_one_fert, wool_contract_repair_v2, wool_family_context_v1, wool_family_context_v2, arlene_v4_m31, salem_sep08_m3, titan_frontier.

Native13: teammate_shoprouter, public_router, public_router_v52, ahmed_v23, junghoon_wool_sales, king_rc4, yusuke_sep08_m2, ahmed_v24, public_router_v5, arlene_v4_m31, salem_sep08_m3, titan_frontier, pass.

Ahmed V25 was ported after this broad protocol. The later cow discovery
uses nine primary opponents, adding ahmed_v25, plus accepted/submitted controls
and PASS. Do not retrospectively claim the old broad audit included it.

## Exact declared gates

1. All required operations, active repair checks, source freezing and full719-action validation pass.
2. Fresh primary utility gain and mean-margin gain have positive95% lower bounds.
3. Fresh historical utility and mean-margin gains have nonnegative95% lower bounds.
4. Native primary utility and mean-margin gains have positive95% lower bounds.
5. Candidate point win score exceeds50% against every primary opponent in fresh and native panels.
6. Every activated missed day guard is investigated; no unexplained animal-service loss or invalid runtime behavior remains.
7. Inspect and report every-opponent win/cash/margin/tail regressions and all candidate-parent comparisons before deciding.

Selection among eligible candidates is highest primary win score, then
primary mean margin. There is no automatic promotion. A guard miss requires an
explicit investigation; completing that investigation cannot waive a failed
numeric gate. Keep all per-opponent losses visible even when aggregate gates
allow some tradeoffs.

## Submitted result and remaining risk

On primary8, q24 raises independent-shop win score78.955%→90.454%, with mean
margin gain167.70 and95% interval[74.63,264.56]. Native win score rises
76.270%→87.891%; margin gain226.47 has interval[-5.89,465.50]. The required
positive native-margin lower bound fails. Accepted reference stays unchanged.

Fresh King wins are955/1024, but relative to the parent this is-5.664pp,
mean margin-9998.37 and worst-decile margin-8625. Native King mean margin
falls8942. In contrast, the Yusuke fresh win gain is88.574pp and mean margin
gain9852. These counterstrategy effects are material, not rounding noise.

All182 missed-guard games were investigated:180 inherited tile38 wheat
divergences and two native PASS unused empty-coop builds blocked by tile73
weeds. No unexplained animal-service divergence remained in that review.
Production loss and failed actions remain in the numeric evidence.

The exact frozen native submission audit against the teammate is4040/4096 wins,
mean margin11793.09; against the previous submission3883/4096, mean3430.81.
Those direct results do not replace the broader multi-opponent verdict.

## Catalog included by this handoff

Historical versions are deliberately included because the user requested the
actual league agents. A catalog entry is not a claim that it is the best agent.
PASS is built into the evaluator rather than stored as an agent package.

| Name | Package | Role in this snapshot |
| --- | --- | --- |
| ahmed_v23 | `agents/external/ahmed_v23` | primary8 |
| ahmed_v24 | `agents/external/ahmed_v24` | primary8 |
| ahmed_v25 | `agents/external/ahmed_v25` | new public threat; later discovery primary9 |
| animal_adaptive_r1_c0_b0 | `agents/external/animal_adaptive_r1_c0_b0` | historical comparison |
| animal_repair_premium_m2 | `agents/external/animal_repair_premium_m2` | broad causal candidate |
| animal_repair_q24_premium_m2 | `agents/external/animal_repair_q24_premium_m2` | submitted candidate |
| arlene_v4_m31 | `agents/external/arlene_v4_m31` | historical comparison |
| binghua_116 | `agents/external/binghua_116` | historical comparison |
| bohann_opening_v1 | `agents/external/bohann_opening_v1` | historical comparison |
| cow_service_retained_q24_premium_m2 | `agents/external/cow_service_retained_q24_premium_m2` | unfinished execution successor |
| crop_mix_t2_wheat | `agents/external/crop_mix_t2_wheat` | historical comparison |
| crop_rotation_t2_berry | `agents/external/crop_rotation_t2_berry` | historical comparison |
| crop_value_m2_t4 | `agents/external/crop_value_m2_t4` | historical comparison |
| early_melon_b98_m1 | `agents/inhouse/early_melon_b98_m1` | independent cold control |
| empty_sale_floor_m1 | `agents/external/empty_sale_floor_m1` | historical comparison |
| empty_sale_slots_m2 | `agents/external/empty_sale_slots_m2` | accepted reference |
| investment_context_guarded_001_best | `agents/external/investment_context_guarded_001_best` | historical comparison |
| john_131 | `agents/external/john_131` | historical comparison |
| junghoon_wool_sales | `agents/external/junghoon_wool_sales` | primary8 |
| king_rc4 | `agents/external/king_rc4` | primary8 |
| late_goose_wheat_context | `agents/external/late_goose_wheat_context` | historical comparison |
| late_value_s32_t0_r05 | `agents/external/late_value_s32_t0_r05` | historical comparison |
| observed_sale_lead_start_216 | `agents/external/observed_sale_lead_start_216` | historical comparison |
| opening_q32_b13_v1 | `agents/external/opening_q32_b13_v1` | historical comparison |
| opening_router_v4 | `agents/external/opening_router_v4` | historical comparison |
| public_capacity_router | `agents/external/public_capacity_router` | historical comparison |
| public_router | `agents/external/public_router` | primary8 |
| public_router_v5 | `agents/external/public_router_v5` | historical comparison |
| public_router_v52 | `agents/external/public_router_v52` | primary8 |
| public_sixday | `agents/external/public_sixday` | historical comparison |
| rival_wool_context_v1 | `agents/external/rival_wool_context_v1` | historical comparison |
| rival_wool_context_v2 | `agents/external/rival_wool_context_v2` | historical comparison |
| rival_wool_context_v3 | `agents/external/rival_wool_context_v3` | historical comparison |
| rival_wool_purchase_repair_v1 | `agents/external/rival_wool_purchase_repair_v1` | historical comparison |
| salem_sep08_m3 | `agents/external/salem_sep08_m3` | historical comparison |
| shop_herd_guarded_001_best | `agents/external/shop_herd_guarded_001_best` | historical comparison |
| shop_herd_s6_m3_g1 | `agents/external/shop_herd_s6_m3_g1` | historical comparison |
| teammate_shoprouter | `agents/external/teammate_shoprouter` | primary8 |
| ticket_p116_t9_i9 | `agents/external/ticket_p116_t9_i9` | historical comparison |
| titan_frontier | `agents/external/titan_frontier` | historical comparison |
| wheat_one_fert | `agents/external/wheat_one_fert` | historical comparison |
| wool_contract_repair_v2 | `agents/external/wool_contract_repair_v2` | historical comparison |
| wool_family_context_v1 | `agents/external/wool_family_context_v1` | historical comparison |
| wool_family_context_v2 | `agents/external/wool_family_context_v2` | historical comparison |
| yusuke_sep08_m2 | `agents/external/yusuke_sep08_m2` | primary8 |

Original manifests and compiler dependency hashes are in evidence/original_agents.json.
Catalog paths are in evidence/catalog.json. The restored configs/league.json
also retains experimental ablations and older specialists outside this45-agent
promotion/current comparison set. Their code remains in the snapshot; they are
not all promoted into the official catalog.
