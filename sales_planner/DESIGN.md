# Contract

The central task is reusable financial planning for the strategy-search pipeline. The first sections state the target contract; the final section distinguishes the narrower retained component. A benchmark measures progress; an integration into the real pipeline establishes usefulness.

## Inputs

`calendar = (period, starting_resources, resource_events, committed_orders, end_requirements)`

The calendar is the financial part of a dated farm plan. It records when production reaches storage, when inputs leave storage, and which paid commitments must succeed. Worker routes and tile choices are outside this interface. A producer supplies either an estimated calendar or a schedule-certified calendar and marks which it is.

Each resource event has a turn, phase, sequence number, item, quantity, and operation. Events are ordered: worker effects before market orders, then day-end deposits. Deposits specify whether overflow is discarded or retained outside storage. Withdrawals cannot use this turn's later market purchases. Seeds have separate storage. Unplaced animals share the 100-item shed.

Commitments specify successful hire and land-purchase counts at each turn. Market-order positions remain planner decisions unless explicitly constrained by the caller. The cold baseline uses source positions as its initial placement heuristic. Required seed, animal, wheat, and fertilizer amounts come from resource needs; their purchase dates are planner decisions. Some warm variants preserve original purchases and label that restriction.

`scenario = (starting_market, future_shops, opponent_calendar, opponent_orders)`

Label each planning scenario as either a forecast conditioned on current observations or a historical stress case. A borrowed historical farm is not a conditioned forecast unless its entry state and obligations are compatible with the observed rival. A separate evaluation scenario remains hidden. The controller receives legal observations, the supplied own calendar and planning scenarios. It never receives the hidden evaluation seed, source identity, future opponent orders, future shop realizations or realized final cash. Supplied planning scenarios may contain their own hypothetical future orders and shops; these are not the hidden evaluation future.

An opponent order tape is conditional evidence. Recompute its accepted quantities, cash, and stock under our changed trades. Record counterfactual input/funding failures; do not treat an impossible opponent continuation as a valid full game. Validate useful changes against live C++ opponents.

## Output

`choose_orders(observation, remaining_calendar, planning_scenarios, memory, budget)` returns market orders and updated memory.

The reusable planner also evaluates a calendar across scenarios and reports final cash margins, unmet obligations, ending resources, and compute cost. A full schedule remains the execution witness; the financial planner does not certify routes or biology.

Initially evaluate from the chosen current turn through turn 718 inclusive, with state 719 terminal. There is no extra day-29 hour-23 sale. Shorter horizons require explicit ending requirements and a continuation value before claiming full economic value.

## Boundaries

- Keep the production plan fixed when measuring the financial component.
- Do not import original sale dates or intermediate cash balances as plan requirements.
- Original purchases are a reference, not the required solution.
- Calendar feasibility and market-policy quality are separate measurements.
- A missing feasible witness is UNKNOWN, not a proof of impossibility.
- Keep strong valid schedules available when search or repair fails.
- Calendar variants from one source family stay in the same data split.
- Fixed replay calendars can encode adaptive future choices. Original-world replay is diagnostic; primary transfer uses calendars fixed before fresh futures.

## What the retained component has actually solved

The tested timing component takes a supplied own resource calendar and an existing order plan. It changes output sale dates within a day, using current public market information and conservative rival stock/delivery bounds. It retains paid farm commitments and checks the full affected game afterward. A compiler supplies our resource flows; finance does not choose our worker routes or tiles.

For the public fixed-plan benchmark, a funded schedule can additionally supply exact purchase quantities and dates. `CaseTurn::purchase_witness` replaces each requested purchase quantity with the amount that executed in that schedule, retaining its order position. A separate normalization check proves this leaves the original game unchanged. These future own quantities are declared inputs. The benchmark does not pretend to infer them, remove their information from farm actions, or optimize the purchases. The broader purchase-date problem above remains open.

A local timing edit is accepted only after its projected work resources and ending private stock and shared market inventory match the existing order plan. This is a proposal check under its stated forecast, not a proof for every rival continuation. Complete live games and fresh public plans separately measure actual feasibility, margin, losses and prediction errors. Earlier stock guards, price-floor effects and cash-dependent purchases explain why both stages are needed.

Seed prebuy experiments deliberately permit earlier seed availability while requiring unchanged other work and ending resources. Their current economic gain is unproved. Do not silently apply that relaxed intermediate-state contract to the retained sale-only comparisons.
