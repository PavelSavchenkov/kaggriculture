# Improve our strongest Kaggriculture agent

Give this whole file to an AI working in a checkout of this repository. It is a
self-contained work request: the AI does not need the original conversation,
attachments, or the author's uncommitted experiment. Paths below are relative to
the repository root. Historical results are starting evidence, not a substitute
for checking the agents available in your checkout.

## September 8 continuation package

Read `handoffs/sep08_agent/readme.md` first. It contains the full restored pipeline,
source/evidence inventory, exact deployment rebuild, working and failed ideas,
league, promotion gates and concrete unfinished work. It supersedes September 7
labels of “current” or “last submitted” below.

- `agents/external/animal_repair_q24_premium_m2/` is the last submitted policy:
  IDs 56101451 and identical repeat 56102764, both COMPLETE. The exact frozen
  C++ version won 4,040/4,096 native games against the teammate.
- `agents/external/empty_sale_slots_m2/` is still the accepted research reference.
  The submitted agent's broad win gain did not pass the required native-margin
  confidence gate. Keep its King regression visible.
- `agents/external/cow_service_retained_q24_premium_m2/` is a useful discovery
  successor: cheaper cow schedules, unchanged wins and investment choices.
- `agents/inhouse/early_melon_b98_m1/` is the independent cold reference and
  remains much weaker than the mainline.

Use `handoffs/sep08_agent/reproduce.py` and `tools/run_comparison.py` to restore
and test the loop without the original session folder. Study all 45 listed
comparison agents and the newer public-port evidence. Keep source provenance,
branch activation, estimated-versus-realized errors and failed attempts.
The most useful next tasks are general post-edit purchase funding, rival-flow
forecasting, broader complete farms, and general continuation repair.

## Objective and working agreement

Work for 24 hours from this run's start, unless I provide a different duration or
stop you. Develop the strongest practical agent we can, together with a runnable
improvement loop. Record the actual start time, deadline, and current objective.
If your environment supports a persistent goal, set it to this objective; report
its real state rather than assuming that a written plan created one.

Start implementing and testing. Continue through failures and routine choices
without repeatedly asking for confirmation. You may change the design, search
method, representation, module boundaries, detail level, and priorities whenever
evidence suggests a better route to the objective. Explain consequential changes
and preserve the evidence. Do not stop after producing a plan, one small gain, or
one submission. If continuous execution is unavailable, leave a precise resumable
checkpoint and state what actually ran.

The main intuition to investigate is this:

- A farm's composition over time should explain much of its strength. Represent
  which products exist, their counts, and their start and end days: for example,
  one cow from day 5 through day 28, or two wheat tiles from day 1 through day 20.
- Estimate the economics and practical service needs of alternative compositions
  cheaply, before finding every worker movement. Use good service and placement
  patterns learned from strong players, approximate daily labor and trade timing,
  and enough resource accounting to reject unworkable plans.
- Spend exact placement and scheduling effort on promising candidates. Compile
  them into complete agents and compare estimated with realized performance.
  Use discrepancies to improve both the estimator and the implementation.
- Repeatedly improve against a growing league containing our strongest teammate
  agents, strong public agents, useful replay reconstructions, specialists, and
  our own previous versions. Keep learning from fresh top-player replays.

The result should beat our strongest available teammate reference and improve
against the broader league, including its own parent. Report remaining losing
matchups honestly. A higher score against PASS or one convenient rival is not
enough. Search for real gains in composition, production, investment, adaptation,
and market decisions as well as execution efficiency.

## Start from the checkout you actually have

Read the applicable `AGENTS.md`, then:

- `prompts/local_agent.md`
- `prompts/experiments_pipeline.md`
- `prompts/game_rules.md`
- `agents/README.md`, `agents/inhouse/README.md`, `agents/external/README.md`
- `fast_game_engine/README.md` and its parity documentation
- `day_solver/README.md`, `day_solver/docs/input_output.md`, and
  `day_solver/docs/integration.md`

If present, read `handoffs/sep07_agent/readme.md` and its
`docs/original_goal.md`, `docs/development_history.md`, `docs/learnings.md`,
`docs/metrics_and_versions.md`, `docs/submitted_lineage.md`, and
`docs/reproduction.md`. This is a dated snapshot. Its description of the latest
submission or unfinished work may be older than the current agent catalog. Treat
archived instructions as historical context, not current requirements. The current
repository instructions and this request govern the new run.

Discover agent packages recursively through `agent.json`. Inspect their source,
provenance, validation scope, and actual entry points. Useful starting references
in the September 7 catalog are:

- `agents/external/bohann_opening_v1/`: the strongest validated local policy at
  September 7, 17:20 UTC. Start here unless newer evidence establishes a stronger
  agent. It combines adaptive herd investment and wheat/tomato/berry continuations
  with Bohann Wang's opening market orders. Read its `PROVENANCE.json`,
  `RESEARCH_VALIDATION.json`, `VALIDATION.json`, and `tests/README.md`.
- `agents/external/investment_context_guarded_001_best/`: the previous broad
  reference and last submitted policy from this session; retain it as an opponent.
- `agents/external/teammate_shoprouter/`: the retained C++ port of the teammate's
  strongest shoprouter agent.
- `agents/external/king_rc4/`, `agents/external/public_router_v5/`, and the other
  materially distinct public controllers listed in the catalog.
- The Atakan portfolios, strong replay phenotypes, and independent strategies
  under `agents/inhouse/`.

Do not infer strength from a directory name, notebook title, or the word
"inhouse." Our best locally improved policy can have substantial external
lineage and therefore belong under `agents/external/`. Bohann opening v1 won
969 of 1,024 direct games against the investment agent and against its immediate
crop-mix parent, 1,001 against the teammate, and 1,006 against King RC4 on its
fresh promotion panel. Its opening also lost two wins against public V5 relative
to its immediate parent; it is not uniformly better on every metric. The package
contains additional native-RNG checks and exact comparison with the frozen source.
These are local matchup results, not live leaderboard ratings or a promise that
the hierarchy holds in a newer checkout. Read the full evidence and rerun it.

The original `external/` tree may be absent because it is ignored. Use the
self-contained catalog ports when available. Do not depend on the live
`experiments/v6/sep07_compositions_v0/` tree, private attachments, absolute paths
from another machine, old binaries, or uncommitted caches. A provenance document
mentioning an old path does not make that path a build dependency. Follow actual
includes and build inputs. Reconstruct missing useful components from available
sources and documented behavior; identify what was unavailable.

Create a new `experiments/<version>/<date>_<name>/` directory and keep its code,
agents, evidence, scripts, builds, and ledgers there. Reuse stable repository
resources such as `agents/`, `agents/common/`, `fast_game_engine/`, and
`day_solver/`. Do not reference another live experiment. When adopting necessary
experimental code from an archive, import the required source into your own
experiment, preserve attribution, and make its dependencies explicit.

## Implementation and resource rules

- Implement local policies in C++ using the repository agent format and typed
  API. Prefer C++ for computation-heavy estimation, planning, search, and game
  batches, with direct interfaces to the engine where useful. Python is appropriate
  for offline orchestration, downloads, analysis, reports, and a generated
  submission adapter. Choose implementation boundaries from measured needs.
- Run Python, Kaggle, notebook, package-management, and build commands through
  `conda run -n kaggriculture <command>`. Follow local compiler and runtime rules.
- Keep code concise, use direct Python imports, fail on unexpected states, do not
  add `__init__.py`, and do not add compatibility fallbacks without a concrete
  reason. Use `rg` for targeted discovery; avoid dumping generated action tables.
- Do not use Git unless I explicitly request a Git operation. Do not create new
  official packages in `agents/` unless requested; experimental agents still
  need `agent.json`, `README.md`, `source/agent.hpp`, and `source/agent.cpp`.
- CPU C++ is the default for this work. There is no requirement to use a GPU.
  This explicitly replaces any generic instruction to use the RTX 5090 just
  because it exists. Preserve unrelated training jobs. Use spare GPU capacity
  only if a measured batch-training or scoring workload justifies it; deployment
  must work on CPU within the competition limits.
- Respect the machine's existing load. Parallelize independent evaluation batches
  when useful, bound resource use, and avoid multiple workers editing the same
  sources. Do not spend the run compiling or launching a new process per game.
- Do not upload to Kaggle or contact other people without an explicit request.
  Researching accessible public notebooks and replays is part of this task.

Keep all episode state in independent agent instances and clear it on reset.
Use immutable shared tables only. Both agents must act on the same pre-step state.
At runtime use only the legal observation history: time, own private resources,
public farms, observed shops and market, and public configuration. Never expose
the environment seed, actual future shops or weeds, opponent-private inventory,
instrumentation, full live state, or live match simulator to the policy.

Equal observable histories must produce equal decisions until new information is
revealed, under the same policy budget and permitted internal randomness. Sampled
belief states may represent unknown futures; they may not read the actual future.
Keep offline full-state diagnostics clearly separate from deployable features.

## Learn deeply from existing agents and fresh public evidence

Before writing another weak baseline, investigate our retained teammate and
Kaggle-derived agents. Read their active controller, branch conditions, embedded
payloads, routes, market layers, reserves, repairs, and terminal behavior. Check
whether differently named agents are actually distinct. Establish source parity
for a port before calling it a faithful conversion.

Fetch a current leaderboard snapshot and recent games from roughly the current
top 12, including new leaders and useful near-top players. Refresh the evidence
during the run; a saved leaderboard date must not stand in for today's field.
Look for newly published or updated strong public notebooks. Compare source or
payload hashes and active code, because metadata-only changes and duplicated
notebooks do not necessarily provide new agents. Convert promising new policies
to C++ and add them to the league after checking fidelity and strength.

You may borrow useful public strategy components: complete courses, individual
day schedules, tile placement, compositions, investment choices, branch logic,
inventory reserves, service patterns, and trading or recovery code. This is
explicit permission to build an externally derived composite for our use; do
not let a preference for locally invented behavior exclude a strong candidate.
Preserve applicable source notices and restrictions, and do not relabel copied
external policy blocks as wholly in-house work.

For every imported component record the player or author, episode and seat,
submission or notebook version when known, retrieval time, source location,
raw/source hashes, exact copied material, local changes, and parity scope.
Distinguish observed behavior from an inferred rule. A replay is one realized
course, not proof that you recovered the donor's complete adaptive controller.

Extract dated, realized information from replays:

- Crop and animal lifetimes, placement and replacement dates, per-tile layouts,
  structures, land openings, ages, production, and harvests.
- Feed, care, watering, fertilizer collection and application, harvest service,
  skipped service, escapes, weeds, failed actions, and repairs.
- Accepted purchases and sales, deposits, worker carry, shed occupancy, discards,
  seeds, feed reserves, hire counts and costs, and trade timing.
- Revealed shops, actual demand by product, prices, public rival state, cash and
  resource constraints at decisions, and end-of-game liquidation.

Reconcile flows and cash. Gross sales are not production: an agent may buy and
resell wheat. Separate production, purchases, sales, net market flow, and discarded
stock. Count crop-days and animal-days from actual lifetimes, including exits and
incomplete tails. Compare like-for-like opportunities rather than only totals.

Keep some opponents or replay sources unused and unanalyzed until a final audit.
Research and development data should not silently become unseen evidence later.

## Search over farm compositions and observable branches

Use a compact typed description of economic intent. Start with dated product
counts or lifetimes, then add the decisions needed to express known strong
families: crop rotations, animal investment dates, land, capital and feed reserves,
placement alternatives, service choices, and conditional continuations.

Keep decisions at whichever level makes the search most effective. Exact routes
need not become outer parameters. Equally, a weak greedy placement or scheduling
routine must not silently eliminate useful alternatives. Move decisions between
search and planning routines when experiments justify it; increase or reduce
detail as needed. There is no prescribed module architecture or parameter count.
Deterministic does not mean optimal.

Use both warm improvement and cold starts. Explore inserting or removing whole
product families, choosing zero of a product, replacing a major part of the farm,
changing lifetimes and rotations, and rebuilding several coupled investments.
Try independent construction from an empty farm when useful. Reserve real effort
for larger changes; do not spend the whole run on one animal substitution or
small tape edits. Build support as experiments require it, without making a
universal composition framework a prerequisite for improving the agent.

Animal choice is general: goose versus cow versus sheep versus waiting, at
multiple useful investment opportunities. Consider adding several animals,
changing their timing, or investing in crops, land, or labor instead. More observed
milk demand should make cows more attractive; more wool demand should make sheep
more attractive. Test that intuition with fully supported branches. It is a prior
to investigate, not a rule that ignores purchase cost, maturity, feed, labor,
remaining days, existing herd, or market saturation.

Policies may branch on observed shops and their quantities, prices and inventory,
public opponent composition and behavior, available cash, weeds, and execution
state. Account for demand rates and remaining consumption time, not just shop
names. Never use an opponent's catalog identity as an unavailable runtime input.
If two rivals are indistinguishable at a decision, a legal policy cannot choose
different openings merely because the offline runner knows their names.

Preserve a diverse archive of complete behaviors. Deduplicate equivalent compiled
policies where practical; report when many parameter settings collapse to the
same actions, branch selections, or economic outcomes. Equal final cash alone
does not establish full behavioral equivalence. Retain useful specialists and
counterstrategies even when they cannot replace the broad incumbent.

## Build a fast estimator that earns its place

Estimate composition versus composition and composition versus an opponent model
across many possible shop histories. Aim for cheap decisions such as whether to
buy a goose, cow, sheep, or nothing without replaying all future worker movements.
Measure full evaluation latency and ranking quality; do not impose an elaborate
model or ML system before simpler C++ formulas and event simulation are tested.

At useful fidelity, cover:

- Exact dated biology where cheap: crop ages, yield windows and caps, water and
  fertilizer timing, repeated harvests and replanting, animal maturity, production
  intervals, banked care, feed, collection capacity, and escape.
- Greedy but improvable tile placement, access to the shed, structures, land,
  travel and service work, pickup/deposit trips, order slots, and daily workers.
  Crossing crops or putting an animal farther away may be useful. Do not hard-code
  a neat layout as the only allowed family.
- Dated cash and inventory: animal and seed costs, wheat and fertilizer supply,
  input availability, shed capacity, worker carry, practical first deposits and
  sale times, and terminal stock or immature investments.
- Marginal labor, including increasing hire prices and whole-worker thresholds.
  A few extra actions can require a costly additional worker; a nearby job can
  fit for free. A constant cost per visit is only an approximation to calibrate.
- Shared market inventory and price changes from both farms, town and shops,
  ordered quantity trades, and price floors. Value an added animal together with
  its effect on output from the existing herd and on rival revenue.
- Plausible unseen shop continuations using the verified game distribution.
  Compare simple expectations with a small deterministic or stratified sample.
  Nonlinear prices mean an average future can value choices differently from an
  average over possible futures.

Productive service is the default prior: aim to obtain useful crop and animal
output, water when needed, feed and care when valuable, collect usable fertilizer,
and harvest before losses. It may work in most situations. The suggested "95%"
is intuition, not a measured constant. Let cheap local search or exact play
override service when prices, labor, capacity, remaining production days, or exit
value justify it. Do not turn full daily service into an inviolable constraint,
or assume every omission in a top replay was deliberate optimization.

Use several levels of fidelity if they help: quick biological/economic screening,
rough placement and labor, sampled markets and opponent response, then exact
scheduling and full games. Reusable day templates, caching, pruning, beam search,
local search, minimax, or deeper lookahead are options, not requirements. Increase
depth only when value estimates and execution are reliable enough to benefit.

Record predicted own cash, rival cash, margin, output, inputs, labor, sales timing,
and uncertainty for candidates that reach exact testing. Check ranking, branch
regret, false rejections, and selected-policy results, not only average prediction
error. Keep heuristic estimates distinct from mathematically valid upper bounds
and feasible exact results. Better forecast accuracy is not automatically a better
agent, and a finite set of tested branches does not bound all possible strategies.

Opponent models need more than a fixed production total. Our trades may change
an opponent's early cash enough to alter its hiring, purchases, routes, future
output, or branch. Use exact counterfactuals to find those cases and improve the
model where they affect decisions. Keep diagnostic use of exact future shops or
trades offline; use it to locate errors, never to provide privileged agent inputs.

## Turn candidates into complete, faithful agents

Compile economic intent into purchases, placement, dated work, inventory
transfers, deposits, sales, and guarded continuations. Start with verified donor
templates when useful, but keep developing construction beyond a fixed library.

Use the existing day solver for suitable fixed-day physical problems. Read its
actual contract and use the typed C++ interface where practical. The current V30
package handles fixed 24-hour days on the 10×10 board with 1–40 workers; it ignores
shed capacity and leaves sales, prices, cash, and opponents to its caller. A strict
schedule inside that contract is not a proof that the whole strategy is funded,
capacity-safe, or profitable. `UNKNOWN` on a budget is not proof of infeasibility.

For a fixed composition, optimize tile swaps, placements, routes and workers, then
the timing and order of purchases, fertilizer use, deposits, and sales. Inspect
how current cash and rival behavior affect intraday choices. Do not optimize
only terminal cash in a no-opponent world and assume the result transfers.

Every composition edit must rebuild or revalidate all its consequences. Adding a
cow affects its purchase, pasture, tile, pickup and placement, wheat, feed and care,
fertilizer, milk, workers, deposits, storage, sales, prices, later cash, and later
route preconditions. Changing one crop can invalidate several later day schedules.
Reusing a route requires equivalent relevant state and obligations, not merely
the same day number or a shared action prefix.

Compare requested and realized lifetimes, per-tile/day output, service, input
flows, and sales. A legal agent that fails to realize the proposal is a compiler
failure, not evidence that the intended composition is weak. Separate economic
rejection, physical infeasibility evidence, timeout, estimator error, compiler
failure, and runtime repair failure. Preserve counterexamples.

Use explicit entry guards for reusable schedules and continuations. Check required
tiles, ages, inventories, worker context, positions, land, and other relevant
conditions. Physical guards alone do not certify financial compatibility. Keep a
legal bounded fallback and report when it activates; do not silently change the
strategy or weaken guards just to improve a headline result.

## Run a repeatable league improvement loop

Implement and actually run this cycle:

1. Freeze the incumbent and a reproducible league. Establish complete baseline
   games, identify losses and economic gaps, and inspect fresh external evidence.
2. Choose a concrete hypothesis and useful initial proposals. Include incumbent
   edits and independent or substantially rebuilt compositions.
3. Estimate alternatives cheaply across common shop scenarios and opponent models.
   Save predictions, rejection reasons, diversity, and timing.
4. Compile a diverse set of promising candidates, plus controls that can expose
   estimator mistakes. Rebuild affected schedules and market plans completely.
5. Run complete exact C++ games on a discovery panel in both seats. Compare with
   the parent and league on identical seeds, not unrelated score samples.
6. Diagnose prediction/execution gaps and use causal ablations to identify which
   component helped or hurt. Update the estimator, compiler, or proposal design.
7. Freeze selected candidates before testing on fresh promotion data. Promote
   only after competitive and operational checks pass under a declared rule.
8. Add useful versions to the league and repeat. Preserve difficult rivals and
   specialists so improvement does not become overfitting to a shrinking field.

The league must contain the strongest available teammate, strong public controllers
including newer ones, useful top-player courses, independent families, and prior
versions from this run. Audit those choices; many near-duplicate tapes must not
silently outweigh one distinct difficult controller.

Define the objective and opponent weighting before promotion tests. A reasonable
starting objective is win utility `(wins + 0.5 * ties) / games`, averaged with
explicit weights across teammate, public-controller, replay, and prior-version
groups. Also report each opponent separately, mean own and rival cash, mean cash
margin, and the lower tail. Do not silently drop a new strong public controller
because it was absent from an old group formula. An objective or league change
must be explicit and compared on a common panel where possible.

Report strict wins, ties, losses, games and distinct seeds, both seat results,
margin distribution, confidence intervals, faults, aborts, discards, terminal
residue, and branch/guard activation. Use seed-clustered paired uncertainty when
both seats or several opponents share a seed. Ties are not strict wins. Local
win rates and cash are not Kaggle ratings.

Small panels are for screening. For a plausible final improvement, use many
complete direct games against the frozen parent and teammate: roughly 1,024
games per important matchup is a starting scale, and 4,096 or more may be useful
for small gains if throughput permits. Choose sample size and stopping rules
before inspecting results; report uncertainty instead of extending a test until
it turns positive. Keep fresh promotion seeds and some final audit cases unused
during tuning. Rerun the incumbent on the same cases.

A promotion rule should require complete operational validity, positive evidence
for the declared league objective, and direct-parent evidence, while exposing
every material opponent regression. Prefer broad improvements. If a worthwhile
tradeoff requires changing an earlier no-regression rule, explain the change and
freeze it before a new independent test. Do not rewrite a failed gate after seeing
its result. Retain an unpromoted specialist when appropriate.

PASS is useful for diagnosis and absolute economics. If using the earlier session's
PASS score, define it explicitly as `J = 0.8 * mean_cash + 0.2 * lower_CVaR10_cash`,
where lower CVaR10 is the mean of the worst 10% of outcomes. It is not a replacement
for adversarial league results.

## Operational and causal validation

Follow all required checks in `prompts/local_agent.md`: generic and typed pair
builds, full PASS and self-play games, debug action validation, independent episode
state, deterministic fixed-budget behavior, legal observations, and matching
generic/pair actions and rewards. Check serial/parallel consistency where used.
Use the official/native RNG path and official environment checks appropriate to
the policy. Verify engine version and configuration rather than trusting a stale
speed benchmark. Preserve action-level parity scope for imported agents.

For causal comparisons, hold seeds, seats, opponent, configuration, and source
versions fixed. Compare outputs, purchases, sales quantities and timing, own cash,
rival cash, hires and costs, failures, and repairs. Isolate components such as
shop selection, opponent valuation, day schedules, or opening orders. A large
gain may come from changed opponent behavior rather than increased own production.
Explain the mechanism supported by the evidence and label remaining hypotheses.

Profile actual bottlenecks before optimizing. Use batched native games, reusable
memory, typed calls, deterministic candidate order, cached immutable data, and
content-based build keys. Include source dependencies, compiler, flags, engine,
API, and CPU target in those keys. Keep the legal fallback available under hard
decision deadlines. Do not use `-ffast-math` or claim a throughput gain without a
controlled measurement. A faster isolated kernel is irrelevant if compilation,
feature extraction, routing, or file I/O dominates the loop.

## Lessons from the earlier session to preserve and retest

These observations should guide experiments, not become universal rules:

- Shop-dependent herd choices did improve complete play. The useful question was
  how to realize the new herd with feed, routes, sales and cash intact. Failure
  of one cow/sheep substitution did not disprove shop adaptation.
- The day solver was useful: one fixed-composition comparison saved 22 hires and
  $1,471 while keeping per-tile/day biology, output, trade quantities/timing and
  rival cash equal. Later context-specific days saved another 11–12 hires and
  $1,076–$1,165 in affected games. Labor savings are valid contributions, but the
  broader composition objective remains.
- Cheap biology and course ranking were useful, yet an early compiler produced
  legal actions while losing production. Legality, faithful realization, and
  competitive value must be checked separately.
- Fertilizer that would otherwise be discarded can cheaply improve a crop, but
  an apparently better placement or larger crop package can cross expensive hire
  thresholds. Later schedule guards can erase the anticipated gain.
- Better estimates of future rival crop quantities did not reliably improve
  investment choices. Replanting and trade dates mattered, as did nonlinear
  prices and the difference between own cash and relative margin.
- Sampling unknown shops improved one portfolio but did not improve every agent
  family. A component needs whole-policy validation in the context where used.
- A borrowed opening's wheat trades and initial buffer had large effects against
  a liquidity-sensitive rival, even when own production stayed similar. An
  alternative buffer could improve other matchups while losing much of that
  advantage. Check order sequence, accepted trades, cash cliffs, and opponent
  response; gross wheat turnover is not agricultural output.
- Copying an entire strong replay was sometimes weaker than borrowing only its
  opening or one compatible day. Isolate components instead of assuming the
  donor's complete course is best for our policy.
- Opponent cycles were real. Preserve multiple useful behaviors and always test
  against the latest local version as well as the original teammate.

## Review every 20 minutes and keep the work reproducible

Every 20 minutes of wall-clock work, review the whole objective and reprioritize.
Do not let an endless parameter sweep replace this review. Give concise progress
updates during work, including uncertainty and the next decisive check.

Each review should answer:

- What is the current best validated agent, what is submitted if anything, and
  what remains WIP? What changed under comparable metrics?
- Which original ideas have evidence: composition search, cheap valuation, dynamic
  placement, full compilation, general animal choice, shop/opponent adaptation,
  cold construction, growing-league improvement, and learning from fresh replays?
- How do we compare with current strong players on animal/crop milestones,
  service per animal-day, output per seed and productive crop-day, fertilizer on
  productive opportunities, net flows and trade timing, labor and cost, land,
  storage, weeds, faults, and discards? Explain cohort and scenario differences.
- Did the last gain come from production, labor, market timing, opponent impact,
  or a branch change? What failed, and was the failure economic or implementational?
- What is the main remaining competitive gap or bottleneck? Which experiment will
  most cheaply resolve it? Should a module, representation, or priority change?
- Are we still exploring independent families and larger changes, or only polishing
  the inherited course? Is the league current and the evidence genuinely unused
  where claimed?

Maintain a short entry-point README, the full objective, a current-best pointer,
an ideas ledger with positive and negative strategy learnings, a separate profiling
ledger, component lineage, experiment results, periodic reviews, objective coverage,
and a precise next-action checkpoint. Preserve failed candidates and their useful
counterexamples without presenting them as promoted agents.

For each meaningful candidate, keep source and manifest, parents and component
origins, configuration, build/run commands, compiler and environment versions,
seeds and seat mode, opponent versions, raw or losslessly compressed results,
analysis scripts, source hashes, and the decision made from the evidence. Freeze
exact tested source dependencies before promotion. Do not rely on mutable parent
headers or an old executable to define a supposedly frozen version.

A teammate with only committed files must be able to reproduce the work. Reference
already committed game-engine and day-solver packages with paths and hashes; do
not bloat a handoff by copying them, their builds, or irrelevant archives. Include
the new agent and genuinely new pipeline components, and document how to recreate
generated artifacts. Keep historical submission bytes immutable and distinguish
them from later local improvements.

## Submission and final deliverables

Submission is a separate action requiring my explicit request. When requested,
read `prompts/kaggle_submission.md` and prepare exactly the selected frozen agent.
Store the submission and its reproduction inputs under
`submissions/<date>-<submission-name>/`. Resolve any older layout examples using
the current recursive agent format. Use a deterministic offline exporter; do not
replace the adaptive C++ policy with one favorable fixed tape.

Before uploading, validate the actual archive in a clean official runtime for
complete games, both seats, PASS and independent self-play. Compare packaged
actions and rewards with the frozen C++ policy across relevant branches, and
re-test that exact C++ version against the teammate over many games. Confirm the
packaged version retains the claimed advantage. Record hashes, counts, timings,
failures, and all source dependencies.

If asked for one submission, make one upload after these checks, record its ID
and receipt, wait for a terminal status, and inspect the server validation replay
and logs. Do not treat an upload receipt as operational success or automatically
retry with a second upload. A submission milestone does not end an independently
authorized improvement run.

At the end, leave the strongest validated complete C++ agent, a runnable search
and evaluation pipeline, a diverse version/league archive, reproducible evidence,
full component lineage, positive and negative learnings, and clear remaining gaps.
Answer directly: did we beat the teammate and our parent, on how many games, by
what metrics; what adaptation and production changes actually work; what the day
solver contributed; and what is still unproven. Do not claim the full composition
framework is solved just because a useful branch or schedule was improved.
