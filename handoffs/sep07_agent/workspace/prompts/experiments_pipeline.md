# Experiments Pipeline

Unless told otherwise, sessions working in `experiments/` must:

- Keep a ledger of positive and negative strategy-search learnings, ready to be summarized into a new strategy design.
- Keep a ledger of positive and negative best practices for profiling ready strategies and their implementations for weaknesses, ready to be summarized into a new strategy design.
- For PASS-opponent strategies, normally optimize `J = 0.8 * mean(cash) + 0.2 * lower-CVaR10(cash)`, where `lower-CVaR10(cash)` is the average cash from the worst 10% of games.
- Use warm starts to refine strong strategies and cold starts to explore independent approaches and escape local maxima.
- Every 20 minutes, review progress as a whole and decide whether to reprioritize the ideas ledger and/or pivot. The review must include deep invariant checks against aggregate patterns common to the globally strongest known external agents, selected by performance in the normal fully enabled game rather than by current-league strength or local executability. League opponents count only if they independently meet that global criterion. Check animal and crop milestones, animal-days and service per animal-day, crop occupancy and output, gross purchases and sales, net sales, sale and purchase timing, labor, land, fertilization, weeds, faults, and discards. Separate real production or conversion gains from transaction-count inflation. Treat universal gaps as approximate optimization targets, compile their full dependencies, check `J` periodically, and accept changes only through common-seed causal ablations and experiment gates.
