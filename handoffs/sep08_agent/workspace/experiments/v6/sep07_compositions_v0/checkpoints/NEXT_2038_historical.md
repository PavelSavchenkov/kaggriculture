# Current state — 2026-09-07T20:38:05.800496+00:00

Goal remains active through September8,00:47UTC. The preceding goal turn is
progress: four-way composition selector promoted, larger independent validation,
and a new strong public controller ported and checked. No Git, new official
catalog copy, push or Kaggle upload is authorized. On-disk AGENTS is current;
retired strategy guide stays retired. CPU is appropriate; leave unrelated GPU
training intact. No subagents. All tool processes are terminal at this checkpoint.

CURRENT REFERENCE CHANGED20:22:53UTC:
late_value_s32_t0_r05, runs/late_portfolio_001/proposals/late_value_s32_t0_r05.
CURRENT_REFERENCE.json and results/late_portfolio_validation.json are authority.
Prior reference q32 is preserved. Committed catalog remains Bohann and last
submitted remains investment_context_guarded_001_best; no external change.

The new reference adds day13 retain-crops/goose/cow/sheep choice on cell31
(tile1,3). It preserves q32's day12 tomato intention (<2tomato shops permits
this wheat replacement) and actual day20 observed berry rule. Shared immutable
calendars, one per-instance q32 base. Compare actual compiled future daily
trade plans with fixed seed/animal/hire costs,32 conditional shop samples and
public opponent herd effects. Score=mean margin minus0.5sampledSD, choose wait
unless positive. No live Sim/seed/future observation/private rival state.

runs/late_portfolio_001/README.md is the pipeline entry. data.hpp generated from
244raw guard/action files; model.hpp/policy.hpp implement value and execution.
Cheapest shared pre20 routes require byte-identical physical day problems;
reusing cow_on_day18 removes another$144 from off-cow. Initial3second schedules
were not minimum labor. Longer solver savings remain in late_animal_schedule_001.
Goose saves8/$898 per activated game; off still one extra$89 onday22UNKNOWN120.
Sheep both and on-cow need no extra hires; off-cow after shared18reuse still
has unresolveddays22/23. All imported HIRE orders have n=1 (1119distinct input
orders). Engine actually executes1hire per slot regardless n; don't generalize
model's n loop to arbitrary raw tapes without normalizing or changing it.

8192counterfactual games (four forced choices×twoopponents×1024) on previously
used1790000 diagnosis seeds. Every prediction prefix equal across choices.
Goose rank correlation.90–.92,MAE$70–$88; cow.85–.86,MAE$441–$449; sheep.47–.56,
MAE$451–$647. Median all-choice32-scenario evaluation.25–.34ms.42simple grid
settings led to halfSD penalty. Actual selected C++ policy matches2048complete
counterfactual records; forcedwait/goose matchold reference/context256records.

First1830000..1830511 fresh panel45056games: only positive current-group95%
lower bound unresolved. Policy frozen, then INDEPENDENT1850000..1852047 panel
180224games across22opponents and two policies, no pooling/tuning. All gates
pass: current group.9451293945→.9467956543,95%gain[+.0007446289,+.0025849152].
Historical group.9496968587→.9519755046,95%gain[+.00139872,+.00317383]. All paired
means positive. Direct q32:1527W2256T313L/4096,+$173.97656. Teammate4046W50L,
98.7793%. PublicV5 3396W700L. Legacycrop variants lose up to1.0742pp andabout$19
worst-decile margin, within preregistered2pp individual limit; all tradeoffs
explicit. Current strong public routers improve~1.37–1.44pp.

896operational generic/pair/debug/thread/self/PASS checks; the native
run has18cohorts×256=4608games including8nativeopponents andPASS perpolicy.
Native/PASS have no mean/utility regressions.222dependencies frozen at
runs/late_portfolio_validation_001/frozen/FROZEN.json, rebuilt frozen pair
matches256native full records. FINAL_AUDITS.json, promote.py and final report
record causal profiles. On256 custom games versuspublicrouter choices
163retain/77goose/12cow/4sheep; vsq32 165/75/16/0. Tomato/berry/melon output stays
unchanged. New local64 profile and all82review metrics generated at20:25.

Useful binaries:
- faace656c5bb3083d227/arena: selector,forcedcourses,q32,publicrouter.
- be4f535e7d37be913503/arena: pair selector/q32.
- 874c538d0e22567a693d/arena: debug selector/q32.
- runs/late_portfolio_001/build/probe: four forced-policy game/prediction export.
- frozen/arena_rebuilt: exact promoted pair from frozen source closure.
Paths under EXP/build unless otherwise stated. All through conda env.

NEW STRONG PUBLIC SOURCE READY:
research/refresh_2002 contains72 fresh top-player games; nextrefresh21:02UTC.
Fields of Fortune is byte-identical4d72dc70... despite new metadata.
Thomas kaggriculture-95-5-win-rate-via-replay-routing is distinct V5/2.
Five719-turn tapes; one exactlyoldV5tape0 and four new. Ten72-turn blocks:
at144 ifpriorroute0, wool demand>0 selects1, otherwise milk demand>0 selects2,
else0. At288 ifpriorroute2, TOMATOinventory<=9916 selects3 else4. Other blocks
retain route. Only features2(tomatoinventoryoffset),24(milkdemand),25(wooldemand)
are referenced; no opponent feature directly used by this controller.

league/public_router_v52 is repository-format C++ port, namespace
kag::agents::public_router_v52::Agent, per-instance route/step/block state,
selected_route() getter. Source literal hash82412467...5a97; exact source and
payload under research/refresh_2002/notebooks/thomastschinkel/... . IMPORT.json
records limits: original replay episode provenance not supplied, no separate
license supplied, user authorizes borrowing. 95.51% is unverified author claim.
12230 source action cases pass, allroutes plus9916boundary, resets/rewind/end.
896 generic/pair/debug/thread/native/self/PASS checks pass.4096discovery games:
newV5/2 beats oldV5 181–75/256,+$2298; loses63–193 to our promoted selector,
margin-$2657.87. Beats teammate256/256; King222/256. New source is a validated
league opponent, not our best. No need another permission request to use it.
Generic ee33d9ed834ac8d4f2dd/arena containsnewV52,oldV5,portfolio,q32,teammate,
King,public_sixday. Pair760853c0b5aaf5e7f0d5 newV52/oldV5; debug04b04635138941a0ac6a.

IMPORTANT NEXT DIRECTION: larger sheep-heavy composition / independent family.
In its63 wins over us, newV52 often has0goose/6cow/11sheep byday12 (34games),
versus its more usual3/9/5 or5/6/6. Mean hires280.54 versusours264.25. Source
profiles in research/refresh_2002/v52_discovery show much wool/berry output.
This is a concrete source to learn from, beyond the single extra-animal slot.
Investigate dated buys/places/service/trades and physical compatibility with
our earlier prefix. Don't assume same opening means identical typed actions
or physical start states; tapesfirstdivergence1 (some3/151) despiteauthorphrase.

Possible next bounded work:
1. Compare q32/portfolio and V52 physical states aroundday6 to see if a source
   suffix can be spliced with small stock/route closure, or requires a full
   independent starting family. Preserve future intent as well as state guards.
2. Start from V52's larger wool/milk calendars, estimate economics across shops,
   use day solver to improve their much higher labor costs and repair any
   useful placements/service/trade timing. This is independent cold-family
   work relative to our incumbent. Consider full donor courses and additional
   earlier purchases, not only another day13 tile.
3. Diagnose sheep valuation by separately evaluating actual future shops offline
   (oracle diagnostic only, never policy input) to separate forecast uncertainty
   from rival-flow/intraday errors. Current estimator's FERTdemand bug in old
   default path is avoided because supplied scenario demand has0fertconsumption.
4. Broaden greedy placement/joint herd search when expected gains justify exact
   routes. Counterfactual selection cache is now proven and cheap.

Original wording reread20:31. Full goal still includes dated outer composition
search, cheap biology/economics/service, placement, exact labor and intraday
trade optimization, measured feedback, all animal/wait choices, large/cold
changes, borrowing top-player components and growing league. Single-slot
success is not completion of arbitrary construction. Review56 includes all82
metrics for new reference; nextreview20:48UTC. LastGLOBAL2002; nextrefresh21:02.
Used seed ranges1790000,1810000,1830000,1850000 and1853000;1863000forV52checks.
Final900000 remains unused. Check before choosing new ranges, e.g.1870000+.
