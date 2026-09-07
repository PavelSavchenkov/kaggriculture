# First financial evaluator

economics.hpp takes dated warehouse arrivals, withdrawals and fixed capital/
workforce costs. It is a conditional financial schedule evaluator, not yet a
complete composition estimator. Biological, placement and workforce modules
must supply these events. Input purchases can supply later turns, never earlier
unit actions in the same turn. A one-day input reserve and immediate surplus
sales are initial candidate rules, with their value still unmeasured.

The model charges marginal per-unit prices, applies shop and town demand,
respects the special $1 sale rule and tracks capacity losses. Counterparty
flows are fixed quantities. Same-turn aggregate rival buys precede sales,
and own input buys follow surplus sales; original order-index interleaving
is not reconstructed. Exact paired play must judge those approximations.

Negative cash, missing scheduled inputs and excess order slots are explicit
witnesses of an unrealized proposed schedule. Negative cash is retained for
diagnosis, not treated as a feasible loan. A funding gap may be repairable by
retiming purchases/sales or changing workforce; it does not prove the economic
composition is impossible. The model conditions future arrivals on execution
and therefore cannot substitute its final cash for an exact promotion result.

Four complete 719-turn financial fixtures match the official C++ engine for
cash, market inventory, sold quantities, residue and discards. Fixtures cover
scheduled wheat supply, simultaneous sales, floor pricing, overflow and a
funding witness. These are market tests, not worker-schedule tests.

Detailed exact profiles now export warehouse deposits/withdrawals and fixed
costs. Nightly automatic deposits are dated to the next market opportunity.
Per-step shed mass balance is checked against the exact post-market state.
scripts/export_financial_cases.py converts the saved data without introducing
policy logic. Forecasting an alternative sales/reserve policy on recorded field
flows is a warm optimization screen; its cash difference from the original
policy is not, by itself, estimator error because the sale policy changed.

Next: replay original trades to isolate price/interleaving errors, evaluate
alternative market schedules in exact C++, and integrate approximate placement/
labor with the biological model. Track those error sources separately.
