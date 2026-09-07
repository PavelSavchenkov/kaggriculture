# Sealed result

Objective:

```text
J_solo = 0.8 * mean(terminal cash) + 0.2 * lower-CVaR10(terminal cash)
```

The final combined candidate scores `J=105460.078` and `105459.878` on the
sequential 32768-seed blocks starting at 1 and 100001. Two untouched shuffled
65536-seed blocks score `105454.957` and `105454.454`; absolute score therefore
has visible cohort variation and should be reported as a range.

The final residual hire rule was compared with an otherwise identical stack:

| cohort | delta mean | delta CVaR10 | delta J | wins / ties / losses |
|---|---:|---:|---:|---:|
| base 19000001, 65536 seeds | +0.252 | +2.520 | +0.706 | 510 / 65018 / 8 |
| base 20000001, 65536 seeds | +0.265 | +2.647 | +0.741 | 523 / 65006 / 7 |

Cluster-bootstrap 95% intervals for delta J are `[+0.641,+0.769]` and
`[+0.676,+0.807]`. The rule improves the requested mean/CVaR objective but is
not Pareto: 15 of 131072 paired seeds lose.

Other retained paired gains:

- day-23 exact-neutral route slack: about `+1.22--1.36 J`;
- opportunity-cost funding versus unprotected purchases: `+116.9--118.3 J`;
- four-day wheat-only lookahead versus five-day wheat/melon:
  `+0.095--0.110 J`;
- opportunity selector versus static egg: bit-identical over 196608 episodes;
- underfilled-sale substitution: one `+$30` result and 262143 ties.

All 524288 episodes in final independent cohort evaluation had zero discards
and route aborts. Seed 16060896 seat 0 finishes at `$104894`, sells the surplus
fertilizer, and no longer discards it. Both seats reproduce exact no-weed
`$105492`. Release and ASan/UBSan smoke tests pass. Peak independent validation
RSS was 129352 KiB with 16 threads.
