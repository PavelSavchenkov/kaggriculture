# Necessary carried-input conditions

Inputs are a validated public DayProblem and an explicit 23- or 24-phase horizon. This calculation ignores the proposed workforce and source schedules. It uses the fixed non-hire purchases to construct the same earliest 39-slot hire menu as the estimator.

For every carried input, count its consuming field actions: feed consumes one wheat, fertilize consumes one fertilizer, and place consumes one animal. Public tile actions have quantity one. For each distance r from the nearest of the four shed-access cells, count the consuming actions at distance at least r.

Relax every possible field output to be freely available at dawn, even if its real production is late or requires the same input. Add initial shed stock. Ignore all withdrawals and other uses. These relaxations overstate early supply and therefore cannot make the condition too strong.

At a purchase release t (purchase hour plus one), subtract all this relaxed stock and purchases released strictly before t from the input demand. Sum positive deficits over items. At least L of the consuming actions need goods released at t or later. A worker doing any of these actions must first pick up at the shed at or after t, then travel at least r steps. Goods cannot be passed between workers in the field: PICKUP reads the shed, and DROP or non-animal PLACE deposits only at a shed-access cell. Worker actions precede purchases within each phase in the engine.

For a worker born at b, an optimistic upper bound on the number of these actions it can perform is max(0, H - max(t,b) - r - 1). This allows every remaining action after one pickup and one outward trip to consume an input; it ignores further travel, chain prerequisites, extra pickup types, and all other work. The farmer has b=0; a hired worker has b=hire_hour+1. The sum across k workers must be at least L. The first k satisfying every cut is a necessary workforce bound.

If even a worker born before t has zero remaining capacity and L is positive, the required consumption is unreachable regardless of workforce. If total relaxed supply after all purchases is smaller than total consumption, that is a separate inventory impossibility. A return of 41 without a positive missing count means only that the implementation's 40-worker capacity is insufficient for this relaxation; it is not an unlimited-workforce infeasibility claim.

Only purchase release times need checking: between releases, early supply is constant and the earliest following release imposes the strongest late-use restriction. The implementation combines items within a release/radius cut because each required consuming action uses exactly one carried item. It does not combine different radius cuts or release times by summing them.

Seeds are global stock and workers can be positioned before their purchase; they are intentionally outside this carried-input calculation. Land releases and input-dependent output deadlines also remain outside it. The result is a conservative screen and structural feature, not a schedule or feasibility certificate.

First audit: `runs/supply_study_v1` contains 6,205 input variants representing 3,739 physical contracts. Of 4,455 source/compiler witnesses, 4,445 pass independent strict replay; the other ten are the previously known historical metadata failures. There are no contradictions on valid witnesses. Answer-workforce and tile-order invariance are checked on every input. The timed call includes the hire menu and averages 1.065 us, with p95 1.543 us, excluding parsing and subsequent audit checks.

The screen proves 39 additional synthetic contracts and ten additional development layouts unreachable. It tightens five other synthetic lower bounds, all on still-unresolved cases; it does not improve the point error on already certified synthetic cases. The immutable first v1 evaluation remains unchanged.

`check_supply` passes 42 controls, including actual engine pickup/travel/fertilization at the last reachable purchase hour and one phase too late, at distances zero through four and both horizons. Early stock and freely relaxed local production prevent false rejection. Additional controls check shared late action capacity and total input shortage.

The unchanged initial reference sweeps spent 2,394.12 CPU seconds on 663 queries for the 39 newly screened synthetic contracts, and 388.76 CPU seconds on 107 queries for the ten newly screened layouts. These are measured past compiler costs that this condition can avoid for those inputs, not an estimate of full-pipeline speed or a result on an untouched post-freeze wave. The per-contract screen itself averages about one microsecond.
