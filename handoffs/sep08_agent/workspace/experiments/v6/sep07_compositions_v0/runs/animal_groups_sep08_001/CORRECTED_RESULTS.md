# Corrected crop and animal opportunity costs

281,256 proposals;36 full source/control games;4242 exact crop lifetimes and317 exact animal lifetimes.

Estimator time 14.695seconds, averaging52.25microseconds per proposal including32shop samples.

The screen now subtracts already planned animal output, feed, fertilizer and work. 
It credits canceled purchases only up to remaining future buys. Animals already 
bought are sunk inventory. A changed tile can replace or advance a planned animal; 
its edit count is not necessarily a net herd increase.

| Initial fixture | Original margin estimate | Corrected margin estimate | Planned animals replaced |
| --- | ---: | ---: | ---: |
| cow_7 | +2784.31 | +2784.31 | 0 |
| cow_pair | +4722.94 | +4722.94 | 0 |
| cow_8 | +2810.28 | +2810.28 | 0 |
| goose_23 | +1260.56 | +261.00 | 1 |
| goose_pair | +2450.69 | +1032.59 | 2 |
| goose_32 | +1260.56 | +775.69 | 1 |

Original flawed scores and duplicate-build compiler fixtures remain preserved. 
Corrected estimates still require compiled schedules, real funding/storage checks 
and an observation-only branch policy before any playing-strength claim.
