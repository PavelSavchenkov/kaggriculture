# Selection after source review and user correction

The user asked whether minimum-cost workforce planning was already solved by the day solver. The root API fixes workforce and hiring times, but the existing caller already searches workforce sizes. Improving its search is useful yet closely overlaps the prior solver project. Merely adding repeated calls or lifting its one-second hint cap is not the distinct component sought here.

Select **fast marginal labor estimation before routing**. This produces estimates and uncertainty, not worker actions. It runs on many proposals that will never be sent to the expensive scheduler.

## Evidence of usefulness and headroom

- The latest general model charges a constant per field operation. It has no route geometry, input/deposit deadlines, existing workforce slack or discrete hire thresholds.
- The earlier model estimates travel/transport, then divides aggregate work by 22 and applies Fibonacci costs. It does not model which jobs can share routes or which goods are available before deadlines.
- The wheat12 expansion's flat estimate was 2,520 for 252 operations. Initial realized extra labor was 10,425. Known full-course schedule improvements save 3,804, leaving 6,621 extra labor on that improved course. These are historical exposed results and reference upper bounds, not minimum required cost.
- Earlier source-support calibration ranked cash much better than estimated-support calibration. The switch changes both labor and land assumptions, so this is not a clean labor-only ablation. Treat it as motivation, not attribution.
- A useful estimator serves different outer searches: counts/species/dates, placement, service choices, borrowed courses and independent construction. Its speed enables more proposals to be compared; its uncertainty prevents a cheap score being mistaken for a feasibility proof.

## Why this rather than other candidates

Incremental multi-day compilation remains the largest broad engineering gap. A single general compiler project would also choose service calendars, purchases, funding, guards and route repairs. A bounded dependency compiler is plausible, but its fair benchmark needs more interface construction and would still be strongly coupled to routing quality. The labor estimator has a cleaner prediction boundary and existing costly failure examples.

Rival flow forecasting is distinct and valuable, but irreducible future-shop uncertainty and opponent response make reference outcomes harder to isolate. Biology arithmetic already has substantial exact verification and less demonstrated remaining headroom. Market optimization has large past gains but needs a more coupled economic objective. Workforce improvements remain useful offline label work, not this session's claimed contribution.

The main risk is poor reference labels: a found schedule is a feasible upper bound, and UNKNOWN is not an infeasibility label. Address this explicitly with verified schedules, valid lower bounds, interval/censored evidence and a fixed-budget downstream compilation test. Do not train a supposed minimum-cost predictor on raw source hire counts and call it solved.

At each 20-minute review, assess predictive headroom, reference quality, family coverage and usefulness to actual candidate selection. Pivot if the clean target cannot be supported by meaningful evidence.
