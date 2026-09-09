# Current state — 2026-09-07T19:51:38.962364+00:00

Active24-hour goal ends September8,00:47UTC. Current promoted reference is
opening_q32_b13_v1; see CURRENT_REFERENCE.json. Committed catalog remains
agents/external/bohann_opening_v1; last submitted remains investment_context_guarded_001_best.
No new Git, official catalog addition, push or Kaggle submission is authorized.
Current on-disk AGENTS applies; retired strategy guide stays retired. CPU is
appropriate; leave the unrelated GPU job intact. No subagents.

The user's day-solver question is now answered with full-game evidence.
runs/late_animal_schedule_001/README.md is the entry point. Initial3-second
search used9/$987 extra goose worker-days off or8/$898 on. Longer searches save
8/$898 in both. Offday22 retains one$89 extra hire after UNKNOWN120; on has no
extra hires. All sales and rival actions stay equal. One unsold final fertilizer
is left uncollected. Original wording that labor was required was overstated.

Species30/AUDITS.json in that run records fixed-workforce cow/sheep retries:
all extra hires removed for both sheep leaves and on cow; off cow has three
UNKNOWN days. All64 per leaf retain production, sales and rival actions. These
optimized routes have not yet been formatted as separate C++ agent packages.
The initial unoptimized cow/sheep WIP packages already exist and compile.

Two broad43008-game attempts are complete and NOT promoted:
1. late_goose_optimized,1790000..1790511. Initial3328 screen positive, fresh
   broad panel failed. Full-state guard admitted both parent wheat and parent
   tomato intentions.181/1024 direct games lost24tomatoes and averaged-$1005.48;
   intended392 wheat-to-goose games averaged+$199.11. The small negative
   unchanged-crop control was an earlier warning to inspect composition context.
2. late_goose_wheat_context,1810000..1810511. Preserves parent's existing day12
   tomato-shop rule (<2 permits wheat). Direct420W480T124L, utility.64453125,
   mean+$80.35. All21 paired mean margins positive, but grouped utility falls
   .9454834→.9407837,95%gain[-.0076904,-.0020142]. Historical noninferiority and
   individual utility gates also fail. Some small negative investments turn
   old near-wins into losses. Artifacts in runs/late_goose_context_001.
  896generic/pair/debug/thread/self/PASS checks passed, custom and native shops.

Next: general observed-market selection among retain crops, goose, cow, sheep.
Use include/animal_investment_value.hpp (daily whole-farm own trade plus public
opponent herd forecast), or its sampled scenario variant. Build FarmFlowPlan
from actual compiled control/animal daily market orders; separate fixed seed,
animal and actual hire costs. Use empty Biology when comparing two complete
flow plans, avoiding double-counted output. Current estimator nets daily buys
and sells and omits rival crops/intraday feasibility, so measure these errors.
Future berry branch at day20 must use only then-observed shops; at day13 use
conservative or sampled future continuation values, not the realized future.
Preserve parent tomato/wheat intent before any calendar selection. Only enter
courses whose physical day13 guard matches. Choose wait if projected advantage
is not enough. Parent q32 already contains earlier animal/shop adaptation.

Possible efficient policy structure: one always-current q32 base, immutable
shared per-species off/on calendars and daily cash-flow models; select a course
atday13, select its berry continuation atday20. Avoid constructing/shadow-running
four whole parent hierarchies merely to score four alternatives. The agent sees
only AgentObservation; offline tooling may extract schedules from full Sim.
Prove action/guard parity and all required operational checks. Runtime hot path
should be a few microseconds for valuation, not an exact day solve.

The1790000 and1810000 panels are now diagnosis data; never call them fresh again
for a fitted selector. Use a clearly new range, e.g.1830000+ (check prior files).
Final900000 reserve remains unused. Offday22 retiming/reuse failed even with
one PASS→DROP trial; do not block composition selection on the remaining$89.

All19:02 public notebook audits complete in research/refresh_1902/NOTEBOOK_AUDIT.md.
V5 Hybrid has identical five tapes/trees to already ported Thomas V5, with a
small weed repair. Diagnostic C++ league/public_router_v5_repair has896 discovery
games, no changed win counts, mean active-opponent gains0..$6.69 and PASS-$4.86.
It is not promoted as a fully source/operational-validated opponent. Fields of
Fortune disables animal purchases and has weak valuations; retain crop-zone/
greedy-placement ideas, no full port prioritized. Specialists notebook is only
presentation/plots, no executable agent. Next public refresh around20:02UTC.
Review54 has all82 current comparison metrics; next review20:08UTC.

The full original scope remains active: dated compositions, fast economics and
service estimation, placement and labor/trade compilation, prediction-versus-
execution feedback, all animal species and wait, larger/cold proposals, reusable
top-player components and growing-league validation. Do not narrow to labor.
All tool processes were terminal at this checkpoint. No new promotion/upload.
