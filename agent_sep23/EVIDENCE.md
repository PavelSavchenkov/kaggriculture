# Evidence, limits, and failures

## Gates passed by the release

| Gate | Result |
|---|---|
| Intent/API contract | strict binder; authoritative hash recorded |
| Causal data | episode-level splits; own past actions only; confirmation unopened |
| Native compiler tests | 29 passed for the product-fallback lineage |
| PyTorch/C++ parity | 300/300 intents matched; max joint log-probability error `1.33e-5` |
| Precision audit | strict FP32 retained; TF32 changed 117 and BF16 changed 1,114 of 10,320 validation intents |
| Fresh compiler comparison | 52/64 wins, mean margin +9,881.58, episode-clustered 95% CI +6,231.86 to +13,856.07 |
| Final compile coverage | 0 uncompiled dawns, 0 missing commitments, 0 emergencies in that comparison |
| Local-LB games | 29/48 wins, mean margin +5,515.08, 0 missing executed commitments |
| Production smoke test | full 719-turn games, both seats, deterministic reset, valid actions, bounded attempts |

## Local-LB snapshot performance

Seeds 230500-230507 were played in both seats, 16 games per responsive local-LB
opponent:

| Opponent | Wins | Mean margin | Missing commitments |
|---|---:|---:|---:|
| replay-champion | 7/16 | +2,740.25 | 0 |
| ttyn-shop0909-v2-guarded-hysteresis | 12/16 | +9,957.63 | 0 |
| pavel-knn-animal-rules-v1 | 10/16 | +3,847.38 | 0 |

These are development snapshots, not an independent hidden leaderboard. The
sample is small and was used during development, so treat it as directional.

## Latency

The selected production pair was measured over 48 complete games and 34,512
action calls with a 256-attempt budget. Mean action time was 3.791 ms; the
largest single action was 762.081 ms. Agent policy time per game ranged from
1.763 s to 4.325 s (median 2.690 s). Complete-match time had median 3.699 s and
maximum 5.617 s. No game contained an action over one second. Separate native
inference measured 1.371 ms per dawn over 900 dawns.

## Full-game failure profile

Replay continuations show the gap is primarily strategic/economic after the
compiler fixes, not illegal execution. Across 72 continuation cases, the mean
terminal margin deficit versus the replay expert fell with a later handoff but
remained: -22,933.6 from day 12, -12,932.2 from day 20, -5,342.8 from day 25,
-2,818.9 from day 27, -1,164.1 from day 28, and -127.5 from day 29. Executed
missing commitments were zero in these selected suffix strata.

Observed recurring failures:

- **Animal service and collection.** The policy/compiler combination can delay
  feed, care, milk, or clipping and lose capacity/value. In one board-switch
  game it fed 97 vs 121, cared 87 vs 110, milked 168 vs 203, and clipped 12 vs
  0 animals while both trajectories had 10 cows.
- **Crop decay.** Ongoing crops can be under-served when competing work and
  collection load grow. One seed originally lost 64 strawberries; collection
  fixes reduced, but did not eliminate, the loss.
- **Overspending and capacity.** In episode 112062878 from day 20, BC finished
  with 32,824 less own cash than the expert, clipped 128 vs 6 animals, lost 120
  vs 2 crops, and spent 9,172 vs 939 on inputs. In episode 112013186, a service
  variant clipped 131 vs 3, collected 171 vs 294, lost 76 vs 0 crops, and spent
  10,210 vs 2,356 on inputs.
- **Funding forecast sensitivity.** Rival market orders and sales can invalidate
  a plan's timing even if the nominal budget balances. The reactive executor
  repairs many cases but cannot recover every strategic choice.
- **Autoregressive crop drift.** On held-out Majkel labels, 15,398/18,446 crop
  decisions were correct with teacher prefixes versus 14,537 under greedy
  prefixes; 834 later targets became unsupported after earlier errors.
- **Narrow fine-tune overfit.** The stage-2 validation minimum is epoch 21;
  epoch 500 is much worse. More epochs are not an improvement.

## What has not been proven

- The protected confirmation split was not opened, so there is no independent
  final estimate.
- The local-LB set is small and not representative of every leaderboard style.
- Zero commitment gaps do not imply good intent: a weak plan can be executed
  perfectly.
- The later overflow-return compiler variant is not validated enough to replace
  this release.
- The latency tail needs wider hardware and scenario coverage despite passing
  the observed one-second threshold.

## Best next improvements

Keep the model/compiler boundary and measure one change at a time. The highest
value targets are collection/capacity-aware intent features, a training loss
that reduces greedy-prefix crop drift, better animal-service economics, and
funding features conditioned on rival order position. Require fresh full games,
both seats, commitment denominators, native parity, and latency before promotion.
Keep some opponent seeds and confirmation episodes untouched until a candidate
is frozen.
