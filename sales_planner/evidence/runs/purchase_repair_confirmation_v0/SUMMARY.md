# Purchase-repair confirmation

A frozen test of the unchanged one-turn required-input repair covers 128 new seeds, five exposed opponents, both seats and three fixed courses: 3,840 paired branch games. Repair appends a purchase only when the required next-turn stock would remain short even if all current requested purchases succeeded. Existing orders stay in place. It checks current cash and storage in the exact own market projection.

Mean final margin increases $91.54 (95% seed-cluster bootstrap interval $67.60–117.25), own cash increases $21.58 (interval $12.39–31.95), and rival cash decreases $69.96. There are 252 positive, 112 negative and 3,476 unchanged games. Worst margin change is -$102; worst own cash change is -$654. Effects are paired by opponent, seed, seat and course, with all of a seed’s opponents/seats/courses kept together in uncertainty estimates.

Repair acts in 364 games. All have fewer own failed actions and unchanged worker-days; all also change production, with 13 decreasing at least one product. Thus this is recovery of intended work, not an equal-production finance comparison or proof of dominance. Most gain comes from the sheep course: +$264.19 per branch game; cow and goose effects are small. The five opponent mean margins are positive, but own-cash effects are not uniformly positive.

Own forecast-failed branches decrease from 386 to zero. Rival forecast failures remain 2,019 out of 3,840. The scenario library still mismatches many live rival farms. Of 364 repaired actual games, 24 had no own failure in the eight forecast worlds. Forecast feasibility cannot be used as a label for actual successful work; these 24 still had fewer real failed actions after repair, but three lost margin.

Secondary selector results: +$113.12 mean margin versus the same selector without repair (interval $5.98–284.57). Versus the old demand and model choosers, margin intervals still cross zero. No reliable superiority over the established plan choosers has been shown. Preserve the strong agents and keep repair as an experimental reusable primitive.

Mean compile plus eight-world valuation per course is 4.335 ms with repair versus 4.017 ms without it, roughly 8% extra in this paired workload. This excludes scenario loading and actual full-game evaluation.

Evidence: PROTOCOL.json; own_repair/PAIRED_EFFECTS.json, PAIRED_GAMES.json and NEGATIVE_GAMES.json; complete per-opponent control and candidate JSONL files. Reproduce analysis with scripts/compare_portfolio_variants.py.
