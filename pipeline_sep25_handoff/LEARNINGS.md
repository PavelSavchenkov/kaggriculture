# Learnings

Short statements with the evidence in brackets. Details: LINEAGE.md, and the original
`docs/experiment_log/LEARNINGS.md`, `PROGRESS.md`, `IDEAS_LEDGER.md`.

## Game mechanics that mattered

- Both players' market orders are filled slot by slot: slot k of each player is processed together, one
  unit per round at the same quote, prices refresh after the slot. An earlier slot sells higher. Top
  teams order their sales almost arbitrarily. [39/40 mirror games, +11.6k]
- Hires are market orders and share the 10 order slots with sales and purchases; hire wages grow like
  Fibonacci within a day (the 12th hire $144, the 13th $233) and reset daily.
- Melons have no shop demand: the first seller of a harvest wave takes the price; nothing recovers.
- Prices of animal products recover overnight; strong players sell at dawn and hold through crashes.
- Shop draws share the night RNG with weed spawns (one draw per empty tile on both farms): changing the
  number of empty tiles reshuffles later shops for both players.
- Items workers carry at night go to the shed; above 100 they are destroyed.
- Kaggle rules changed on Aug 15: replays before that date do not replay in the current engine.

## Evaluation

- Full games decide; offline metrics mislead (continuations ranked v2 first, games ranked it last).
- Replaying the Local-LB's own seeds predicts its table exactly; a panel on other seeds does not.
- Seeds that selected a setting overstate it (94% vs 88% on fresh seeds).
- 40 mirror games are ~20-30 independent outcomes; one set can flip (25-15, 31-9, 26-14, 17-23).
  Adopt after >= 4 sets including the deciding Local-LB seeds.
- Mirror games against our own lineage cannot show forecast gains; varied opponents can.
- Losses are fragile: a per-game best of 8 settings flips every loss for network pushes and compiler
  variants alike, so whole-game best-of experiments do not tell which part to fix.
- Check which model each side loaded: an environment variable leak made a whole afternoon of
  head-to-heads invalid.

## Network (behaviour cloning)

- Output design beats capacity: count categoricals, marginal heads + exact MAP DP, median/total x
  shares decoding. Argmax of a spread count distribution under-plants; rounding a per-member
  probability blurs whole-group decisions; DP over sequentially trained heads decodes untrained logits.
- Always run the teacher-forced decode check before playing a network.
- More data + conditioning on player strength beats filtering to strong players (+2.1k vs -11.3k).
- Train long (40k steps; 10k steps lost 4.8k), keep the best validation checkpoint, but select
  networks by games: validation loss does not predict strength, and seeds of one recipe range from 3
  to 23 wins of 40 against each other.
- A style one-hot index that means "unknown team" is a trap; use the all-zero input for "no style".
- An opening style pin (days 0-5) is a large, cheap lever; whole-game or mid-game pins are not.
- The network imitates experts well on expert states (teacher-forced totals match); the gaps appear in
  closed loop (budget cuts, compiler safety rules, a smaller herd with idle cash).
- Uniform pushes of the network's decisions all lose, but per-dawn search with an exact opponent copy
  triples the margin: the headroom is in state-dependent decisions.

## Day compiler

- The biggest full-game losses were compiler fallbacks, found by tracing lost games day by day:
  dropping every new entity on an unfunded day (idle farm), survival mode that stopped harvests
  (collapses), trims that removed the day-0 sheep in every game.
- Determinism first: wall-clock budgets made results depend on machine load; fixed counts fixed it.
- The design contract's rules pay off when enforced: new entities must fit free tiles (a 20 s day came
  from 19 crops asked on 18 tiles).
- Speed comes from the formulation, not micro-optimizations: failed market-return re-solves were 75%
  of compile time; searching from the affordable maximum downwards cut 33% with identical decisions.
- Wage-aware decisions must compare a decision's gain with the marginal wages it adds, not all wages.
- Tiles bind more than wages mid-game: deferring a harvest to save a hire delays the replant.
- A one-day horizon cannot hold stock: every bolted-on holding rule overflowed the shed.
- Keep the next-dawn cash reserve mid-game (without it a seed collapses), but not where it forces a
  day-0 trim of a productive animal.

## Engineering

- Ship the model with sidecar files for every setting (opening, condition, compiler options, decoding),
  so several versions of the agent play correctly in one process; environment variables leak.
- Portable bridge: glibc 2.28 sysroot + static C++ runtime; set the model path at game start.
- Record Local-LB games (`OPUS_DUMP`, `LB_DUMP=1`) and replay them without Python (`tools/lb_replay`)
  to debug one dawn with `DC10_DEBUG`.
- One training process at a time (24-32 GB each); ~16 game threads on this machine; screen small,
  confirm big.
