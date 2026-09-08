# Goal and operating rules

The objective is to build the strongest practical agent by searching dated farm
compositions: what exists, how much, on which land, and during which days. Estimate
many candidate continuations cheaply, compile promising ones into real worker
actions, then use differences between predicted and realized results to improve
both estimation and execution. Repeat against a growing, diverse league.

The user's intuition is preserved in original_objective.md. In particular:

- Search over whole crop/animal lifetimes and composition suffixes, not only
  route polish or one cow-versus-sheep substitution.
- Compare goose, cow, sheep, crops, mixed herds, earlier/later purchase, removal,
  and waiting. Branch on shops already observed and useful public rival evidence.
- Use productive watering, fertilizer collection, feeding and care as strong
  defaults. They can be overridden when approximate or exact local optimization
  shows that the exception pays. They are not unconditional rules.
- Estimate land, placement, labor, feed/fertilizer, liquidity, warehouse limits,
  production timing, deliveries and sales as well as nominal final output.
- Value changes in the context of the existing herd and the opponent because
  both players change shared prices. More output or more own cash alone is not
  a sufficient improvement.
- Reuse strong openings, layouts, lifetimes, full day schedules and branching
  logic from top-player replays and public notebooks when useful. Record exact
  donor/source/version/hash and distinguish copied behavior from inference.
- Keep cold construction and larger family changes alive to escape inherited
  farm assumptions. Improve or move module boundaries when evidence warrants it.
- Review progress every 20 minutes: completed results, real live jobs, failures,
  bottlenecks, whether the current choice set has enough headroom, and next work.

Read the repository's current AGENTS.md, prompts/local_agent.md,
prompts/experiments_pipeline.md and prompts/game_rules.md before continuing.
The old strategy-search guide was intentionally removed; do not recreate it.
The original session's time window is historical context, not an instruction
to spend several days without the teammate's own time or compute agreement.

All Python, Kaggle, build, binary and package commands use
`conda run -n kaggriculture`. Use `--no-capture-output` when stdin is needed.
Agents and online policy decisions remain C++. Python is for orchestration,
data processing and the final Kaggle adapter. CPU work is the default; no GPU
workload was needed, and an unrelated GPU training job was left intact.

Legal observations contain own/private inventory and public farms, market,
shops, time and config only. Never expose a real game seed, future shops,
opponent-private inventory, live Sim or diagnostic state to the agent.
Oracle future-shop/flow experiments are offline error-attribution tools only.

The episode ends after 719 transitions: step718/day29/hour22 is the last action.
A padded hour23 used to certify a day endpoint is not an extra live action.
Keep source hashes, seeds, both seats, and all attempted variants. Do not rename
a failed audit, reuse its seeds as “fresh,” or change gates after seeing results.
Final seed block900000 remained unused in this snapshot.

Do not use Git or submit externally without an explicit request. This handoff's
commit/push was explicitly requested; it does not authorize future submissions.
Store further work in a self-contained experiment. It may use persistent root
resources and official agents, but may not depend on another live experiment.
