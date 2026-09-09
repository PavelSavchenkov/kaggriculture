# Fixed-calendar group screen

281,256 proposals across 18 shop scenarios; 36 full current-agent/control games. 
Both complete action hashes, money, production, sales and discards match in every control. 
All extracted crop lifetimes reproduce their exact harvested output.

14.582 seconds inside the estimator; 51.84 microseconds per proposal including32 shop samples. 
This excludes full-game source extraction, process startup and file output.

| Animals added | Proposals | Positive score before labor |
| --- | ---: | ---: |
| 1 | 4,848 | 1,386 |
| 2 | 41,409 | 8,635 |
| 3 | 234,999 | 48,397 |

Best estimated groups vary with the scenario. These are search candidates, not 
validated recommendations. Full-season compiler results and a legal runtime branch 
selector are still required before any league claim. See ESTIMATION_ANALYSIS.json 
for every selected per-scenario candidate and the explicit model limitations.

CORRECTION: Scores for tiles with later planned animals omit their opportunity cost. Read LIFECYCLE_CORRECTION.json; use the corrected estimate_v2 output once verified. The original counts, timing and unchanged-source controls remain valid observations.
