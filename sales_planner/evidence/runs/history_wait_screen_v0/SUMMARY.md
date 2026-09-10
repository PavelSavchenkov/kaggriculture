# Public-history waiting: development screen

Frozen primary rule: wait one turn across known demand only when public history rules out stored rival output and no rival output is currently ready. Only carrot, tomato, strawberry, melon, egg, milk and wool are eligible. Own cash, storage and next-turn commitments are checked in the two-turn projection. Recorded rival actions enter the evaluator only.

All 558 variant/case/seat jobs completed: 93 episodes, 186 player plans, three variants. Results are relative to original market orders with unchanged requested worker actions and rival orders.

| Rule | Mean margin gain | Episode-cluster 95% interval | Gains / losses / ties | Noncash differences |
| --- | ---: | --- | --- | ---: |
| Public history + current ready | $22.72 | $15.91 to $30.96 | 90 / 0 / 96 | 1 |
| Public history + four-turn ready | $22.25 | $15.47 to $30.52 | 89 / 0 / 97 | 1 |
| Four-turn ready, output products only | $21.22 | $3.55 to $38.88 | 121 / 59 / 6 | 2 |

The primary rule's mean advantage over the output-only old guard is $1.50, with interval -$14.49 to $18.96. Mean superiority is not established. The clearer result is fewer losses. In the 42 new discovery player plans, the primary gains $27.67 on average, with 23 gains, no losses and no noncash changes; the old guard gains more on average ($36.93), but has eight losses, worst -$703.

Keep the exception: episode 107270694, seat 1 changes seed stock from turn 164. Its primary margin gain is $6; production, faults and terminal shed stock match. Therefore this is not an all-case identical-state improvement. No history bound underestimates were observed. Raw-source action exceptions are reported separately from local-agent API validity.

The unchanged candidate was frozen before opening the 17 confirmation episodes. See ../history_wait_confirmation_v0/SUMMARY.md. These tests establish a component result on recorded plans, not reacting-opponent agent value or better plan selection. Machine-readable details: SUMMARY.json, COMPARISONS.json, PAIRED_GAMES.json and NEGATIVE_OR_CHANGED.json.
