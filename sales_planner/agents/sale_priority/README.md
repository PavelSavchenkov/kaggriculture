# Sale priority experiment

Start from `room_keep`. On turns containing only sales, put existing orders in descending order of available sale quantity times current price. Preserve quantities and all other turns. Stable ties retain the supplied order. Uses only the legal observation and own current worker actions.

Motivation: all 206 multi-product sale turns in Otter Vibe's three latest sampled appearances at 23:15 UTC follow this order. See `runs/top_patterns2315_v0/IMMEDIATE_DETAILS.json`. This is an independently implemented observable rule, not copied policy code. It does not assume that leaderboard rank establishes the rule's causal value.

Underlying production and market schedule have the external `early_structure_cow` lineage. Experimental, not submitted; promotion requires full-game evidence and the local-agent checks.

Measured development result: `runs/sale_priority_broad_v0`, 3,072 paired comparisons against room_keep across12 opponents,64seeds,bothseats,two shop panels: +$50.16 mean margin (95%seed interval$33.91–66.17), +1.660pp win utility, no production/labor/extra-fault changes. 969 negative comparisons, worst-$752. Several opponent means remain negative. Keep room_keep as the conservative alternative. This is an exposed opponent panel, not source-family holdout or a Kaggle submission.

Validation: generic, release pair and debug pair agree exactly over eight full games; one/four thread results agree; full self-play and PASS tests pass. New state is per instance; only legal observation fields are used. Sequential alternating timing against AhmedV25: median elapsed ratio1.0037 (about0.4% slower; includes the opponent/engine and possible CPU contention). See `runs/sale_priority_checks_v0`.
