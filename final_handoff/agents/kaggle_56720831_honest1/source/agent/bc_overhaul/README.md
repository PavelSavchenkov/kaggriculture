# bc_overhaul

The frozen bc_opus network decodes one DayIntent per dawn (same decoding, opening style, herd
reach and max_land from `<model>.decode`); the new day compiler `dc11/` executes it.

Parameters: model path (`model_path`, default `models/l3_reach9/model.bin`, see below),
`dc11::Options` (`options_text`, else `DC11_OPTIONS`; "-" = defaults). Work per dawn is capped by
a fixed route-evaluation count (deterministic) and by `DecisionBudget::soft_deadline`: past
either, no further funding variant, trim or herd-reach retry starts. Probe-only environment
overrides: `DC11_MAX_LAND`, `DC11_LAND_PUSH` ("first last bias"), `DC11_REACH` ("first last q ..."),
`DC11_REACH_STRESS` (0: reach plans need only be funded under the expected forecast).
`DC11_DAYLOG=1` prints one line per dawn, `DC11_PLAN=<day>` that day's plan.

Assumptions: the network's DayIntent interface is unchanged; the compiler may drop the least
valuable work when a day cannot fit (reported as `dropped`); the hard deadline is not checked
(one dawn compile takes ~0.1-1.8 s here).

Default model: `models/l3_reach9` (frozen fin_Z_rs_l3 weights; herd-reach quantiles 0.9 0.8 0.7,
reach plans funded under the expected forecast: reach_stress 0; land_match 11 14 50: the 4th
quadrant on days 11-14 when the opponent owns it).

Packaging: land_match reads the opponent farm's n_quadrants from the observation; a Kaggle adapter
must fill it (the existing lb_bridge path does), otherwise the rule never fires.
