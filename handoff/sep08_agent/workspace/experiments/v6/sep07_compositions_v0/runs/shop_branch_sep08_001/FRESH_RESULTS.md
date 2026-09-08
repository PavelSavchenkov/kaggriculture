# Fresh shop-selector result

7,776 full games, fixed rule and source dependencies unchanged.

Versus service_bank_p362_m2: gains (utility,margin,cash)=[0.0263671875, 2396.359375, 1715.1884765625], 95% intervals=[[0.00830078125, 0.04345703125], [1447.8674926757812, 3443.809851074218], [891.411767578125, 2589.5782104492187]].

| Opponent | W/T/L | Margin | Utility gain | Margin gain |
|---|---:|---:|---:|---:|
| empty_sale_slots_m2 | [0, 0, 256] | -56731.67 | +0.000pp | +2468.00 |
| teammate_shoprouter | [2, 0, 254] | -49374.56 | +0.781pp | +2668.33 |
| public_router | [0, 0, 256] | -53288.90 | +0.000pp | +2786.77 |
| public_router_v52 | [0, 0, 256] | -55513.31 | +0.000pp | +2943.39 |
| joint_routes_p362_m0 | [204, 8, 44] | 3962.05 | +1.172pp | +1997.39 |
| service_bank_p362_m2 | [94, 114, 48] | 1918.12 | +8.984pp | +1918.12 |
| cold_renewal_p98 | [117, 52, 87] | 754.89 | +8.984pp | +1918.12 |
| early_melon_b98_m1 | [27, 0, 229] | -7559.50 | +1.172pp | +2470.75 |
| pass | [256, 0, 0] | 116451.17 | +0.000pp | +3507.34 |

Gates: {"direct_reference_utility_margin": true, "active_utility_positive_95pct": true, "active_margin_positive_95pct": true, "each_active_opponent_bounded_regression": true, "all_PASS_won": true, "native_mean_bounded_regression": true, "operational_parity_frozen_sources": true}

Versus early_melon_b98_m1: gains (utility,margin,cash)=[-0.154296875, -5188.55126953125, -1601.67529296875], 95% intervals=[[-0.17822265625, -0.12841796875], [-6155.472717285156, -4209.24287109375], [-2444.1275634765625, -740.7240234375006]].

| Opponent | W/T/L | Margin | Utility gain | Margin gain |
|---|---:|---:|---:|---:|
| empty_sale_slots_m2 | [0, 0, 256] | -56731.67 | +0.000pp | -2722.25 |
| teammate_shoprouter | [2, 0, 254] | -49374.56 | +0.000pp | -2824.49 |
| public_router | [0, 0, 256] | -53288.90 | +0.000pp | -2805.29 |
| public_router_v52 | [0, 0, 256] | -55513.31 | +0.000pp | -3094.50 |
| joint_routes_p362_m0 | [204, 8, 44] | 3962.05 | -10.938pp | -7829.09 |
| service_bank_p362_m2 | [94, 114, 48] | 1918.12 | -31.641pp | -8112.14 |
| cold_renewal_p98 | [117, 52, 87] | 754.89 | -41.406pp | -6561.15 |
| early_melon_b98_m1 | [27, 0, 229] | -7559.50 | -39.453pp | -7559.50 |
| pass | [256, 0, 0] | 116451.17 | +0.000pp | -861.30 |

Gates: {"direct_reference_utility_margin": false, "active_utility_positive_95pct": false, "active_margin_positive_95pct": false, "each_active_opponent_bounded_regression": false, "all_PASS_won": true, "native_mean_bounded_regression": false, "operational_parity_frozen_sources": true}

Retained cold reference: early_melon_b98_m1. Mainline reference unchanged.
