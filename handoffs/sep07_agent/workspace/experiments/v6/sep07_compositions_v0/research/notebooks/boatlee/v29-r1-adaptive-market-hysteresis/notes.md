# V29-R1 | Adaptive Market Hysteresis

V29-R1 keeps a coherent high-output three-quadrant production program and adds a stateful market controller. The production schedule still determines what the farm builds and grows; the controller decides how much premium inventory can be released now without consuming quantities reserved for later sales or animal care.

## Policy design

**Inventory pressure.** The controller combines Shed stock, carried inventory, scheduled demand and public market inventory. Pressure must persist before it changes a sale, so a single noisy turn cannot repeatedly flip the policy.

**Price-gated tranches.** MILK, WOOL, STRAWBERRY and MELON are released in bounded tranches. Item-specific reserves shrink gradually through the season, while a price gate prevents early low-value dumping. Every extra unit is charged against a per-item budget.

**Public market flow.** A decaying estimate of external supply is updated from public market inventory changes. It changes the release threshold rather than replacing the production route.

**Mirror-safe hysteresis.** When both public farms remain close in production shape, labor and land at the decision checkpoint, the controller keeps the conservative scheduled sale path. This avoids two similar farms reacting to the same public signal with the same oversized sale.

**Progressive liquidation.** The reserve and price requirements relax near the end of the season, then switch to a bounded tail liquidation so valuable stock is not left unsold.

## Evaluation protocol

The frozen evaluation uses `kaggle-environments==1.32.7`, both seats, and seeds not used for controller tuning. Historical regression, same-day ladder references, score-stratified sampling and V28 online-loss families are reported separately.

## Frozen Regression

| Version | Games | W/T/L | Paired +/0/- | Mean margin | P10 | Worst |
|---|---:|---:|---:|---:|---:|---:|
| V29-R1 | 108 | 106/0/2 | 53/0/1 | +23,314.9 | +6,447.1 | -20,220 |
| V24-RC1 | 108 | 87/8/13 | 42/6/6 |  |  |  |

The panel contains V19, V20, V21, Moon V56, Kaito V37/V48, Nikita-derived, HealthStone and Rayk C18 routes.

## Same-day Ladder

| Panel | V29-R1 W/T/L | V29-R1 paired | Reference W/T/L | Reference paired |
|---|---:|---:|---:|---:|
| Current Top-20 representatives, 12 seeds each | 196/0/20 | 98/0/10 | V24-RC1 45/0/171 | 24/0/84 |
| Score-stratified 1500–2799, 26 teams | 310/0/2 | 155/0/1 | V24-RC1 145/0/167 | 73/0/83 |
| Higher-fidelity score sample, 12 teams | 144/0/0 | 72/0/0 | V24-RC1 58/0/86 | 29/0/43 |
| Eight V28 online-loss families | 91/0/5 | 45/0/3 | V28-R13B 52/0/44 | 26/0/22 |

The two independent current-ladder batches were `100/0/8` and `96/0/12`. Across the thirteen score bands, only 2500–2599 produced losses (`22/0/2`); every other band was `24/0/0`.

The separate 81-trace V28 loss-pressure audit was `162/0/0`, paired `81/0/0`. Its traces are correlated within route families, so the eight-family panel above is the de-duplicated comparison.

All candidate runs completed 720 turns with both agents in `DONE` state. Candidate exceptions, malformed actions, overlong market queues, escaped animals and seedless plant requests were all zero.

## Production trace

The production trace was reconstructed from completed public replays of RngRng submission `55948382`, including episodes `104679155`, `104689164` and `104691666`. V29-R1 adds the adaptive market hysteresis, reserve accounting, public-flow estimator, near-mirror guard and runtime safety layer described above.

## Agent
