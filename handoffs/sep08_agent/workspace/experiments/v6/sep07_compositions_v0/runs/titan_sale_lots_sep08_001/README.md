# Conditional partial-sale search in C++

Translate the pure `MarketPath` and `optimize_lot` components from the pinned
TITAN release. Read LINEAGE.json, CHECKS.json and the retained LICENSE/NOTICE.
The fixed-array C++ search considers integer quantities sold now, at later dates,
or carried beyond its horizon. It compares own-minus-rival receipts under five
explicit rival timing/supply scenarios, with exact rounded prices and $1 market
admission within those conditional streams.

All216 cases match the reviewed source's chosen plan, candidate count, worst
gain, summed gain and every scenario's own/rival receipts and carry. Cases include
price-floor boundaries, terminal/nonterminal horizons, zero/full100-unit stock,
cash-related minimum-now quantities and a caller capacity constraint. The first
oracle adapter used the wrong shop-ID order; that failure is retained under
first_oracle_mapping_failure/. Correcting the oracle mapping required no C++
change. The oracle executes only explicitly selected pure AST definitions, with
no source imports, notebook, external agent, loaders or network calls.

This is a validated economic primitive, not a complete agent. It assumes default
game prices, at most100 own/rival units, at most8 future turns and four candidate
dates. The caller still needs current post-unit stock, upcoming funding, exact
order positions and a storage/arrival contract. Its scenario minimum is a search
choice, not a guarantee against arbitrary opponents. Integration must preserve
the best fully checked action and honor runtime budgets.

Use `conda run -n kaggriculture python` with check.py or benchmark.py. The benchmark
times the C++ calculation inside one process, separately from environment startup.
No strongest-agent change or external submission is made here.
