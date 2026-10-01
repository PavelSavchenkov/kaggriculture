# M&M imitation plan (Sep 30 02:15 BST, for 48 h; user-approved)

All 4 sessions work only on copying M&M (Kaggle #1). Nothing else unless the user asks. Imitation is the main session: it assigns
work, asks for updates, integrates, and owns this file, GATES.md and DEVIATIONS.md. Sessions talk to each other directly as well.

## Goal: "M&M level"

- Behaviour: on M&M's own positions, our compiler and our network make M&M's decisions (G1, G2).
- Quality: our agent in M&M's seat, against our reacting local agent, reaches M&M's real margin from any hand-over day, and in the
  end from the very start (G3).
- Kaggle guard: no loss on the beds that approximate live play.

## Gates (home: experiments/v10/sep29_mm_copy = X; build X/build_dc12h unless noted)

| Gate | Command | Measures | Pass |
|---|---|---|---|
| G1 compiler | `X/runs/gates/g1.sh <model dir> <tag> [d0 d1 games threads]` (teacher_day: M&M's dawn state + M&M's label intent, our compiler executes the day vs the recorded opponent; score `X/scripts/g1_score.py`) | plantings, land, herd, field, services (yield waterings), weeds, dropped, fallback, thin-product sales per hour band and units, next-dawn value | all checks in X/scripts/g1_score.py |
| G1s seller, reacting | `REC_DP="3,5,6,7" REC_ARM=<model dir> X/runs/duel/mm.sh ...` (M&M's recorded farm + our seller vs live d3crop; see X/runs/duel/queue_sellfid.sh); price per lot: experiments/v10/sep29_dc12/tools/sell_price.py | hour shares, day-average price per unit, margin vs M&M's own selling (rec_s) | M&M's shares, price >= M&M's, margin >= rec_s |
| G1-seller (fast, teacher-forced) | `X/build_dc12h/seller_diff <list> d0 d1 threads out.csv` (BC_OPUS_MODEL = a model dir's model.bin for its .dc11 keys): at every hour of M&M's days our choose_sales gets M&M's exact state and picks this hour's lots; score `X/scripts/seller_score.py` (201 games x days 10-27 in 32 s) | hour-band shares, P(sell) by h%4, lot size, same-hour overlap (daily units are inflated where M&M carries stock) | M&M's per-hour decisions (for I2 fits / I1 / rules) |
| G2 network | `X/runs/gates/g2.sh <model dir> <tag> [d0 d1 games threads]` (options_diff: the model's decoded intent on M&M's dawns vs M&M's label; score `X/scripts/g2_score.py`, per day `X/scripts/g2_days.py`) | new crops per type, animals, land ask, per-group options, value-weighted | crops +-5% per block (+-10% per type), animals +-0.1 / day, land 95%, options <= $100 / day |
| G2-own trajectory | intent log + ops in the arm's own games (`DC11_INTENTLOG=1 DUEL_OPS=...` on duel_mm, e.g. X/runs/duel/intent60 on the 60 quick worlds, `DUEL_STOP_DAY` to cut; X/scripts/intent_split.py) and Weaknesses' traj.py on G3-wide (arm vs M&M's own recording in the same worlds) | asked vs planted vs M&M's label per day and crop; herd, plantings, production, sales by block | per block within 10% of M&M (plantings / herd / production) |
| G3 takeover | `X/runs/duel/pin_sell.sh <model dir> <tag>`; hand-over at dawn X: `DUEL_REC= DUEL_LOAN=1 DUEL_REC_UNTIL=X`; read `X/scripts/pin_cmp.py` (6 exact M&M-vs-our-sub worlds; more coming from Weaknesses) | margin vs M&M's real +9.49k; own vs opponent income | whole game >= M&M's margin; no losing block |
| Guards | league `X/runs/league/league.sh` (+ `X/scripts/league_pool.py`); swap bed (Weaknesses, work/sep29_validation/swap); pinned field bed (Day compiler) | margin | not negative |

Attribution: G3 shows the losing block; on its first dawn the state is exactly M&M's, so G2 and G1 there split the loss into
network asks, compiler executing M&M's own intent, and compiler dropping our network's intent.

## Owners (one owner per code tree)

- Imitation: gate tools, baselines, GATES.md, DEVIATIONS.md, triage of every fail, integration of passing changes into one package
  (network + compiler together), G3 whole game.
- Day compiler: compiler tree (experiments/v10/sep29_dc12): seller hours / lots / prices, land hour, same-day planting on new land,
  funding, intent drops, collection execution. Iterates G1 / G1s until pass.
- BC (kaggriculture-8b): network tree (experiments/v10/sep29_bc_mm): crop asks (main vs members, solo / landpush), land-day plantings,
  cash-sensitive land ask, collection intents, checkpoints / conditioning / fresh M&M-heavy data. Iterates G2 until pass.
- Weaknesses: pull fresh M&M games from Kaggle (now, then every ~3 h), M&M reference profiles (gate targets from all M&M replays),
  growing the exact M&M set (every new M&M-vs-our-agent game, exactness-checked), held-out M&M lists for G2, guard beds, live
  monitoring, triage support from replays.

Split: BC and compiler work run in parallel. When one component passes its gate (or is blocked on the other), its session takes a
parallel line on the other component (e.g. a second compiler variant line in its own snapshot), assigned by Imitation. Weaknesses
takes triage / replay-analysis items from DEVIATIONS.md when its data work is ahead.

## Working rules (user)

- Never idle. While runs go, think, triage, design the next variants, read logs.
- Never "one idea -> submit to a gate -> wait 10-20 min -> iterate". Screens must return in seconds to ~1 minute: run only the days
  and games that the idea touches (gate args d0 d1 games), several variants at once in the background, and read partial output as it
  comes. Full gate runs only to confirm.
- Triage every fail to its root cause (logs: seller decisions, intents, land hours, member outputs, collections).
- Generalise: when a pipeline assumption disagrees with what makes M&M win, remove the assumption and rebuild that part. No patches on
  top.
- CPU: ~16 game threads for the team; one BC training at a time.

## The loop, per idea

1. Take the top open deviation in your domain from DEVIATIONS.md.
2. Triage to a root cause from logs.
3. Design a conceptual fix.
4. Screen on your fast gate (G1 / G2 subset), iterate until it passes.
5. G3 ladder, then guards.
6. Hand to Imitation for integration; Imitation reruns all gates on the combined package (changes interact).
7. Any fail goes back to triage with the gate output as evidence; DEVIATIONS.md records root cause and what was tried.

Every 30 minutes each session: update findings/<session>.md and send Imitation a short status (what passed / failed, next step).

## External ideas (user, Sep 30 10:05)

Every session keeps reading /home/pavel/Programming/kaggriculture/codex_ideas/astra/ (passes/; files arrive over time, written by another
agent). Take the ideas in your area, triage them against DEVIATIONS.md and the stop lists, test the promising ones through the gates, and
report taken / dropped with reasons in your findings file and to Imitation. Imitation routes cross-area ideas.

## Setup rules (user-approved, Sep 30 10:45; after the 09:06-10:25 model-file corruption)

1. Shared model files are read-only. Never write into another session's dirs. To use someone's model, copy it with dereference
   (`cp -rL`) into your own dir; your arm dirs hold real files, or links only to read-only files. Never `cp` into an arm made by `cp -r`.
2. Every run has a manifest: sha256 of the binary, every model file incl. all sidecars (.style, .condition, .dc11, .gblind, ...), and the
   env flags that change play. A resume with a different manifest is refused. Tool: experiments/v10/sep29_mm_copy/scripts/manifest_guard.sh
   <manifest> <binary> <model dir>... (exit 3 on mismatch); Weaknesses' swap/run.sh and BC's arm_manifest.py do the same.
3. Builds are frozen once used: never rebuild a build dir that runs have used; a code change gets a new dir with SHA256.txt.
4. One integrator: Imitation builds combined candidates (network + compiler keys + seller) and runs G3 / league reads on them. The other
   sessions screen their own component on frozen builds they own and hand passing changes to Imitation.
5. Stall check at every 30-minute review (Imitation): ListAgents + sep29_mm_copy/scripts/stall_check.sh; a busy session silent > 20 min is
   checked for a blocked tool call.
6. Never `rm` a path that starts with a shell variable; never ask the user for approval; never idle.

## Compute and gating rules (Sep 30 10:40; overload found: load ~50 on 28 threads, ~38 game processes, no cap)

7. (SUPERSEDED Sep 30 10:50: one machine-wide gate of 18 processes, work/runq/README.md.) Old: at most 6 concurrent game processes per session (24 total). Imitation sets priorities; a session that needs more asks
   Imitation, who can lend another session's slots.
8. Two-stage gating.
   - Stage 1 (seconds to minutes, on only the days the change touches): teacher-forced G1 / G2 / seller_diff, own-state intent split,
     day-limited continuations (DUEL_STOP_DAY, hand-over), probes.
   - Stage 2 (only for Stage-1 survivors): G3-wide on the 218 complement (compiler-only arms) or the clean 99 (networks trained on G3
     worlds) + the swap bed (all 234), read as margin AND half-tie score. The 760 is a collapse check only (its evening-follower clones
     disagree with the live-like beds); the league reports our-lineage fit but does not decide alone.
9. Before any Stage-2 run, write down the expected effect and the n needed for 2 SE; don't run knob sweeps whose expected effect is
   below that.

## Compute reorganisation (user-requested, Sep 30 11:00): supersedes rules 7-9 above

See work/runq/README.md. In short: every game / screen process runs through work/runq/slot.sh (18 processes for the whole machine, 4 kept
for Stage-1 screens); Stage 1 first; Stage 2 only for survivors, registered in work/runq/PLANNED.md, one candidate per session at a time,
bed order G3 218 (clean 99 for G3-trained nets) -> swap 234, read with work/runq/seqtest.py at 24 / 48 / 96 / 192 paired games and stopped
at the first REJECT / PROMOTE; league and 760 only for finalists.
