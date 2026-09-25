# Learnings

## Strategy and model (positive and negative)

- The biggest full-game loss was in the compiler, not the network: when an early
  day could not be funded, the fallback dropped every new crop and animal, so
  the farm idled at the start. Trimming new entities one unit at a time (most
  expensive first) turned 14/64 wins (-18,062) into 42/64 (+4,867) vs agent_sep23
  with the same model. Continuations from day 20 could not show this.
- Output design matters more than capacity. Mistakes made and fixed:
  1. Whole-farm counts decoded by argmax of a 101-way distribution collapse to 0
     when "none" is the single most likely value (new crops under-planted).
     Median decoding or a total-count + type-share factorization fixes it.
  2. Group fields modelled as one per-member probability (target k/n) and decoded
     as round(n*p): a shortcut for variable group sizes that blurs whole-group
     decisions. Experts act on whole groups 84-98% of the time (feed 92%, care
     98%, retain 97%, harvest 84%). Use a categorical over the concrete count.
  3. Count heads trained sequentially (masked by the members still unassigned)
     have untrained logits for counts the teacher path never allows (the last
     option, clear, almost always had 0 remaining). Exact MAP DP decoding over
     such heads cleared whole crops: 0/64, -133,765. For DP decoding, train
     marginal heads over 0..size (v7); for sequential decoding, feed the
     assigned/remaining counts as inputs (v6, as in the Sep 23 BC).
- Always run the teacher-forced decode check (tools/decode_eval.cpp) before
  playing: it caught the DP failure at once (86% member mismatch).
- Width 256 beats 128 on every loss component; it overfits after ~8k steps on
  54k dawns, so keep the best-validation checkpoint.
- Sanity: the network memorizes 512 dawns to 15.0 against an exact entropy floor
  of 14.3; validation stays near 28-35, so the remaining gap is ambiguity in
  expert behaviour and missing inputs, not broken training.
- The 10x10 tile CNN alone did not lower validation loss; product calendars,
  capacity and per-group value/deadline features did (34.9 vs 35.9).
- Continuations and full games rank models differently (v5: worse continuation,
  better full games). Report both.

## Profiling practice

- Trace a losing game day by day: our intent, compile status/fallback and
  reason, cash, farm composition vs the opponent (BC_GAME_TRACE).
- Trace a continuation per day and per hour (BC_TRACE, BC_HOUR_TRACE): cumulative
  discards, carried units, shed, orders.
- Seats of one seed often mirror each other; count independent seeds.

## Open

- Heavy harvest days: workers carry 200+ units into the night; the route solver
  cannot route the end-of-day returns (capacity part 141 units by hour 23 fails),
  and 100+ units are destroyed. Next: staged return targets that follow the
  base schedule's per-hour carried inventory, or a post-route deposit pass.
- Day 29: BC intents trail the replay by ~200-300 per perspective.
- Cross-group coordination (decisions of earlier groups as input, as in the
  Sep 23 BC) and whole-farm autoregression are not implemented.
