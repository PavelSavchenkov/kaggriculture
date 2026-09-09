# Comparisons with the original pipeline estimators

Last updated September 9, 12:36 UTC. Costs are in game cash. Timing and accuracy panels answer different questions; do not combine them into one claimed speedup.

| Test | Original control | Current result | Evidence |
| --- | --- | --- | --- |
| Novel cold layouts, mean time to reference-best bill, 46 pools | Geometry with nearby queries: 13.206 s | Frozen direct-cost model: 1.616 s; 87.76% reduction, positive family interval | `holdout_a_layout_novel_evaluation_v2` |
| Same layouts, best bill within 30 CPU s | 43/46 | 46/46 | Same report; actual C++ scoring included |
| Novel source days, workforce-equivalent MAE, 558 labels | Original geometry: 3.624 | Direct cost: 0.497; signed bias -0.079 | `holdout_a_source_novel_evaluation_v2` |
| Novel removal pairs, marginal MAE, 166 labels | Original flat10: 214.80 | Direct cost: 26.49; 87.67% reduction, positive family interval | `holdout_a_work_novel_evaluation_v2`; 22 censored pairs |
| New owned additions from exposed families, 168 labeled pairs | Original flat10: 82.86 | Frozen direct cost: 64.47; three severe expensive failures remain | `expansion_owned_pairs_v2_complete`; 21 censored pairs |
| New larger additions from exposed families, 134 labeled pairs | Original flat10: 210.17 | Frozen direct cost: 149.39; secondary timing + boost: 81.04 | `expansion_pairs_v1`; 63 censored pairs |
| Complete original warm compiler, 14 paired runs | Original mean CPU: 154.905 s | Necessary screen: 161.641 s, 4.35% higher; common-success bill +58.25 | `warm_screen_benchmark_v1`; gate fails |
| Complete warm compiler with learned deferral, separate 14 paired runs | Original mean CPU: 148.872 s | Guided: 113.313 s, 23.89% lower; one lost certificate and mean common-success bill +21.18 | `warm_guided_benchmark_v1`; speed gates pass, quality gates fail |
| Complete warm compiler, prospective eight-seed transfer, 56 pairs | Original mean CPU: 163.440 s; 30 certified courses | Unchanged guided: 125.854 s, 23.00% lower; 34 certified courses, none lost; mean common-success bill -4.8 | `warm_seed_benchmark_v2`; fixed seed-transfer gate passes, worst individual bill +466 |

The accepted scopes are `search_v2_cost_only_h24_cold` and `warm_defer_002_fixed_fixture_seed_transfer`. The optional cold query model fails its additional-benefit gate. Learned warm deferral passes the new-seed gate with the same source/rival and shop sequence; its earlier exposed-course quality failure remains on record. It is not accepted as a general warm policy across other fixtures.

New family-exclusion fits, deeper solver evidence and exploratory stopping rules remain development results. Their improvements cannot replace the frozen test rows above. Holdout_b was opened after the third candidate freeze; all primary predictions preceded its now-running reference sweep. A separate cold-stage warm candidate remains under another eight-seed paired evaluation.
