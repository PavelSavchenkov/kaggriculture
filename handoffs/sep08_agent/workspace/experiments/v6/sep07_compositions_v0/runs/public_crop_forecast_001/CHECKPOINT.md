# Public crop forecast checkpoint

The temporary submission pause is complete. The public crop forecast and its bounded audit are now finished; see `README.md`, `BRANCH_REPORT.json`, `fresh_1660000/BRANCH_REPORT.json`, `RESIDUAL_REPORT.json` and `FINAL_AUDIT.json`. The original planned scope below is retained for development lineage. No forecast policy was promoted or submitted.

Planned independent scope: observation-only dated rival sale forecast from visible current crop tile kind/species/age/held yield/water/fertilizer expiry, evaluated against sheep and Atakan exact branch outcomes. Keep current-animal projection as a control. Compare explicit productive-service assumptions (known fertilizer window only versus maintained fertilizer) and one-shot harvest dates (max-yield date versus first reached cap). No inferred future planting or animal expansion initially. Unknown private shed, routes, funding, sale delays, replanting and future herd changes remain forecast limits.

Engine source inspected: fast_game_engine/sim.hpp, official1.32.7 source SHA256 bc8a54879ef02c7ea64b8b333d6a976f0ea65c4949149d01f463f23bccee653e. One-shot WATER adds yield only at agesceil(maxday/2)..maxday, ongoing output has exactly four scheduled events, fertilizer bonus depends on previous-day water/expiry, and held caps/decay matter. Do not mistakenly project strawberry/tomato indefinitely.

Inputs available: runs/atakan_oracle_ablation_001 retains320contexts×3exact branches and isolated C++ reconstruction; runs/sheep_expansion_portfolio_001 retains320contexts×2branches, exact donor transition/profile checks and four V5 counterexamples. No future-data oracle may enter the forecast API.
