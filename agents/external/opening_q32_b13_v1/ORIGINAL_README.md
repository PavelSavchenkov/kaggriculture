# opening_q32_b13_v1

Promoted experimental reference after the 20-opponent fresh panel, required
operational checks, native checks, and an additional 19-opening fresh audit.
See `../../../../results/opening_market_validation.json` from this package.

Retains the complete adaptive crop/animal policy from crop_mix_t2_wheat. At step
0, prepend a 32-unit wheat buy/sell round trip and set the initial wheat buffer
to 13. At step 1, reduce the original wheat sale by the initial-stock reduction;
retain all other parent orders and optimized hires. Only legal observations are
used. Reset clears the original-stock adjustment.

The round-trip idea comes from Bohann Wang episode 106497007, seat 0; quantity,
stock and hiring changes come from this experiment's measured ablations. Full
inherited crop, animal, worker and replay lineage remains with the parent.

Original fresh direct Bohann result: 1,020 wins / 1,024 games, mean margin
$8,415.24. Additional independent audit: 1,023 / 1,024 against Bohann and
1,005 / 1,024 against the q81 hire control. Most older-opponent gains are about
$5; two opponents lose one strict fresh win each, and several tails decline.
This is the current experimental reference, not an uploaded or committed agent.
