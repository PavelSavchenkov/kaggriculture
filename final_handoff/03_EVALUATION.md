# 03 Evaluation: how we judged changes, and how the beds misled us

Most wasted days in this project came from a test bed that rewarded the wrong thing. This file lists every bed we ended up
using, what each can and cannot see, the statistical practice, and the judging rules we finished with.

## The beds

All local beds run our C++ engine. "Our agent" is the candidate; "opponent" is whatever the bed puts in the other seat.

| Bed | Opponent | Sees | Blind to | Where |
|---|---|---|---|---|
| **Live Kaggle** | Real teams, reacting | The truth | Noise: ±3k per game needs ~60+ games per arm. The pool drifts within hours | `work/sep29_validation/live/` (band tables, same-window A/B) |
| **Live-game continuations** (one-day / 5-day) | Real opponents' recorded actions from our own live games, from the exact recorded state | Real states, real opponent behaviour, exact identity (the recorded agent reproduces 700 / 700 days) | Effects beyond the horizon; opponent reaction | Day compiler `teacher_day` harness; `code/evaluation/live_continuations/` |
| **Hybrid opponent bed** | A real team's recorded farm, with a learned M&M seller that reacts to prices | Realistic sale hours (wool h8.5 vs real 8.7) and real production | The opponent's farm cannot react | `code/evaluation/beds/hybrid_bed_run.sh`, list `list_field.txt` (99 worlds) |
| **Opponent-replay bed** (recorded real opponents) | M&M's real opponents' recorded actions, with loans | Real opponent hours | No reaction (open loop) | `opponent_replay_run.sh` |
| **G3** | Our package as the live opponent, in M&M's real worlds (M&M's seat) | Real worlds; full reaction | The opponent is our lineage (late seller) | `g3_fixed_opponent_run.sh` |
| **Swap bed** | Our real live subs replaying, in real top-30 worlds | Real worlds; real top-30 seat | Opponent = our older subs (lineage). Leaks if a learned part was trained on these games | `work/sep29_validation/swap/`; lists `list_exact_mix_nocma.txt` |
| **Exact Local-LB judge** | The Local-LB roster, run through the LB's own code | Reproduces the LB's official games to the dollar (83 / 83) | The roster is our own lineage | `code/evaluation/exact_lb_judge/` |
| **Frozen LB replays** | The siblings' recorded LB play, frozen | Sanity check | Open loop: any deviation from the recorded play tends to win | `frozen_lb_replay_run.sh` |
| League / mirrors / clones | Our agents | Fast screening | Lineage only | older folders |

## The biases we found (and paid for)

1. **Lineage opponents sell late.** Every local opponent ran our compiler, so it sold wool at h6–8 in the opening; real teams
   sell at h3–5 (mean hour 7–8 vs 4.5). Days 10–28: real teams sell earlier and about 2x the wheat. So any early-selling or
   denial change looked better locally than live. Example: the opening keys scored +0.9k on G3 and were level on the hybrid
   bed (own −1.8k).
2. **Replays cannot react.** The live gap to M&M is reaction: top-30 teams earn +4.6k more against us than against M&M.
   Fixed-opponent beds showed our agent at M&M's level.
3. **Training leak.** A forecaster trained on our live games gains far more on beds built from those games (+4.3k on
   trained worlds vs −0.8k on untrained ones). Keep bed worlds out of training and out of checkpoint selection.
4. **Lineage fit on the Local-LB.** The LB roster is all our own agents. The exact LB ranked forecasters by how much of our
   lineage's games they were trained on (fc2ens +922 > fc3vens +42 > clean fc3nvens −1,153 vs #1), while the real-opponent beds
   had them equal. CMA decode won the LB and lost live (2709 vs 2808).
5. **Open-loop beds favour deviation.** On frozen replays the opponent keeps doing what it did against the original play, so
   almost any change "wins" (+3–4k for every candidate). Use them only as a sanity check.
6. **Pinned beds hid network gains.** Fresh networks were +3–4k reactive but +0.2–0.3k on the pinned bed, because its clean
   filter dropped most top-team rows.
7. **Held-out loss does not rank game strength** (both networks and forecasters). Select by games.
8. **Time-window confound live.** The pool hardens within hours. The same archive (the base) rated 2807.8 on Sep 30 and
   restarted at 2626 that evening. Compare live agents only on games from the same window.
9. **Winner's curse.** Picking the best of N variants on the same 24–96 games overstates it by ~1 SE. Confirm on untouched
   worlds or a fresh live-game set (set B in the final decision).

## Statistics practice

- **Pair everything**: same world / seed / seat for both arms; report the paired margin, its SE, own and opponent money, and
  decided-game flips. Margin is what wins; own and opponent split tell you how (production vs denial).
- **Mirror ties.** Identical agents tie exactly in many worlds (half the G3 mirror games). Read wins against the mirror's
  score, or read decided flips only.
- **Seed clustering.** The two seats of one seed are correlated, sometimes identical. Cluster the SE by seed: f1's +922 was
  2.4 SE per game but 1.9 SE clustered.
- **Sequential reads.** Stop runs at 24 / 48 / 96 / 192 paired games. REJECT if mean + 1.28 SE < delta; PROMOTE if
  mean − 2 SE > 0 (`code/runq_gate/seqtest.py`).
- **Identity first.** Before any comparison, the base must reproduce a recorded game exactly on the bed's binary. Every
  rebuilt bridge, new binary or package was checked this way.

## Rules we ended with

1. Judge sale-timing, delivery and opening changes on the **realistic beds first**: live-game continuations, hybrid,
   real top-30 worlds. Lineage beds come second.
2. A **Local-LB** push needs the exact judge (90 games: the blocks a new challenger plays) **and** non-negative realistic beds.
3. **No part tuned to our own lineage or the LB roster** (user rule): no lineage-weighted forecasters, no CMA, no RL / PPO
   agents in any role.
4. For Kaggle: **identity + official-environment verify + Kaggle kernel check + one submission + reproduce the validation
   episode** ([06_FINAL_AGENTS.md](06_FINAL_AGENTS.md)).
5. Live reads only in the same time window, and only large behaviour changes are expected to show (+1k per game ≈ 20 rating
   points).
