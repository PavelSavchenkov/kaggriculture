# 07 Process: how we worked

## The team

From Sep 24 the work was done by several Claude Code sessions running in parallel on one machine (i7-14700K, 20 cores,
RTX 5090), each with a role, plus teammates' own lines (RL / PPO, CMA, notebooks).

| Session | Role | Main outputs |
|---|---|---|
| **Imitation** (lead) | Integration and decisions; M&M imitation; test beds (G3, opponent replay, hybrid, frozen LB); Local-LB PRs; Kaggle submissions | `evidence/imitation_LEDGER.md`, `code/evaluation/beds/` |
| **BC** | Networks, decode, forecasters (fc2, fc3nvens), conditioning, the forecaster audits | `code/training_*`, `evidence/team/findings/bc.md` |
| **Day compiler** | dc11 / dc12 keys, teacher-day isolation, live-game continuations, exact C++ LB replay, the late-planting diagnosis | `code/day_compiler/`, `evidence/team/findings/day_compiler.md` |
| **Weaknesses** | Live Kaggle analysis (band tables, same-window A/B, reaction of top teams), swap bed, the exact Local-LB judge | `code/evaluation/exact_lb_judge/`, `evidence/weaknesses/` |
| **Improve Agent** | Package combinations, packaging, kernel checks, deadline emulation, Kaggle bundles | `agents/*/checks/`, `evidence/HONEST_LINE.md` |

Coordination:
- **Cross-session messages** for requests and results.
- **One ledger per session**, each written only by its owner: append-only, with the real time from `date` and the numbers.
- **A shared state file** (`evidence/team/STATE.md`): goal, best agents, open questions.
- **A compute gate**, `code/runq_gate/slot.sh`: every game process takes a slot, at most 18 on the machine, `hi` / `lo`
  priorities, and stop files end a run early at a verdict.

## Rules the user set (keep them)

- Run fully autonomously; never ask for approval; never idle. While runs go, analyse and coordinate.
- **Never submit to Kaggle or push kernels without an explicit ask.** Kaggle quota is 5 per day per team.
- Never `rm` a path that starts with a shell variable (`rm -f $DIR/*`). It blocks for approval even in bypass mode and stalled
  two sessions for hours. Use literal paths or fresh folders.
- Run Python through `conda run -n kaggriculture --no-capture-output` with script files. Conda drops heredoc stdin.
- Local agents in C++ only. Experiments self-contained under `experiments/<v>/<date>_<name>/`.
- **Fix model assumptions, not black-box patches.** Broadcast insights and keep a stop list of closed ideas.
- **Fast screening**: seconds to ~1 min per screen, variants in parallel. Long CPU panels only to confirm the best.
- **Patch-stack incumbent bias**: the current best is a tuned stack. Compare a component on a stripped base and on the full
  stack, ablate leave-one-out, replicate.
- **Carry known fixes across lines**: an execution bug fixed in one line is fixed in every line. Probe "wanted vs executed".
- **Timing changes need a Kaggle A/B** before they enter a submitted agent.
- **No CMA, no RL / PPO agents** in any role (target, opponent, evidence): they are overfit.
- **No part tuned to our own lineage or the LB roster**. At the end of the competition, minimise overfit-prone techniques and
  prefer small, well-validated changes.

## Process mistakes (and the fix we adopted)

| Mistake | Cost | Fix |
|---|---|---|
| `cp -r` of an arm folder kept symlinks; a later `cp` onto `model.bin` overwrote a shared network | One hour of runs on a wrong network (Sep 30) | `cp -rL` / `--remove-destination`; check `find -type l`; shared models read-only |
| Multi-thread jobs (`-n 2`, `-n 4`) each grabbed part of their slots and waited for the rest | 6 of 18 slots locked idle, 66 jobs starved in the final hour | One thread per job (`-n 1`); the gate has no all-or-nothing grab |
| Guessed timestamps in logs | Labels 3–7 h off, twice | Always stamp with `date` |
| An opponent's `main.py` set `BC_OPUS_MODEL` at import, so our challenger silently loaded their model | Invalid head-to-heads | Set the env only around our own `opus_new`; prefer model sidecars to env knobs |
| Comparing live agents across time windows | A wrong "hybrid not better" verdict | Same-window reads only |
| Small-key confirmations while the big levers waited | User frustration, hours lost | Rank work by measured size; stop at sequential verdicts |
| Trusting lineage beds for timing changes | Opening keys / CMA / fclin looked good locally, not live | Realistic beds first ([03_EVALUATION.md](03_EVALUATION.md)) |
| A nondeterministic Local-LB opponent (`latest-yannik-suffix-5d-28s-v1`) | Broken determinism checks | Exclude it from identity checks |

## What worked in the process

- **Decision ledgers** with numbers and times made every claim checkable, and made reversals cheap: we corrected several of
  our own reads within the hour.
- **Identity checks before every comparison** (rebuilt bridges, new binaries, packages): no "improvements" came from build
  drift.
- **Teacher-day isolation** (replay one day from a strong player's recorded state with their intent) separated compiler problems
  from network problems.
- **An independent judge** (Weaknesses' exact LB, Day compiler's live continuations) next to the proposer's own beds caught the
  biggest bias (lineage fit) before it reached Kaggle as a final pick.
