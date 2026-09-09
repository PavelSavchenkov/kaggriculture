# Review55 — 2026-09-07T20:12:56.481535+00:00

Due20:08UTC. The previous goal turn made concrete progress: exact day-solver
savings, complete policies, two broad rejected candidates, and a preserved
stronger reference. The original goal continues through September8,00:47UTC.
No Git or submission. Current reference remains opening_q32_b13_v1.

The general animal/wait selector is now implemented in C++. Four immutable
complete calendars share one per-instance parent controller. At day13, only
within the parent's wheat context and a matching physical guard, it compares
keeping crops, goose, cow and sheep. Values use actual compiled daily buys and
sales, seed/animal costs and achievable hire costs. The model samples32 future
shop paths conditional on observed shops. Each scenario uses its corresponding
day20 berry continuation; the actual policy waits for day20 observations to
make that branch. No future price, seed or hidden opponent inventory is used.

Equal physical day contracts permit reuse of on-cow day18 in the shared prefix,
saving another$144 even when the later off berry continuation is chosen. Source
contracts/hashes are recorded in runs/late_portfolio_001/DATA_LINEAGE.json.
All1119 distinct recorded HIRE orders have n=1; the model's fixed labor sum
matches that compiled representation. The root game executes one hire per
order regardless of quantity, so any future generalized import must preserve
that rule. Current data do not rely on quantity-sensitive hiring.

8192 complete counterfactual games give every investment's actual outcome on
the same observed prefix. Predictions agree across all four forced courses.
Median time for all32-scenario choices is.253ms against public_router and.337ms
against q32. For goose, predicted versus realized margin correlation is.90–.92
and mean absolute error$70–$88. Cow correlation is.85–.86, error$441–$449. Sheep
is less reliable: correlation.47–.56, error$451–$647. Future demand uncertainty
and market-model gaps still matter, especially wool. This is concrete support
for cheap ranking followed by exact execution and error feedback.

A grid of42 simple risk/threshold settings on previously used1790000 seeds
selected expected margin minus half a sampled standard deviation. It chooses
wait/goose/cow/sheep, with sheep only in a small subset against public_router.
2048 exact actual-policy games reproduce every selected counterfactual record.
Forced wait/goose match existing parent/context policies in256 full records.
896generic/pair/debug/thread/self/PASS checks pass, including native shops.

First unused1830000..1830511 panel:45056games,22opponents,two policies. Direct
q32 result380W566T78L,utility.64746094,mean+$155.40625. Every opponent's paired
mean margin improves. Current grouped utility rises.94199219→.94316406 but
its95%gain interval[-.00065918,.00295410] still crosses zero. All other
preregistered gates pass, including historical noninferiority. No promotion.
An independent2048-seed1850000 confirmation is running with the policy frozen,
same gates and no pooling or fitting on the first panel. Native/PASS audits and
a source freeze are also running. Do not describe an unresolved confidence
bound as proof of improvement.

Fresh20:02 top12 replay cohort is complete, and two changed notebooks are
downloaded without executing code. Fields of Fortune is byte-identical to the
previous pull despite new metadata. Thomas95.5% uses five719-turn schedules,
ten72-turn blocks and previous-route tests. Its actual referenced inputs are
tomato inventory, milk demand and wool demand. It is a distinct controller
worth porting and checking; the title's win rate remains an author claim.
Raw data and SHA are in research/refresh_2002/THOMAS_PAYLOAD.json.

Keep the full scope: dated composition search, cheap biology/economics/service,
placement, workforce/trade compilation, estimate-versus-execution feedback,
all animal species/wait, larger/cold proposals and replay borrowing, growing
league and unused audit scenarios. The current four-way selector realizes a
major piece of the original intuition, but one tile/entry date is not a general
cold-start composition search. Expand useful dated proposals after validation.

Below are all82 refreshed global72 versus unchanged promoted-q32 local64
comparisons. These are different cohorts, not paired performance evidence.
Next review20:28UTC; next public refresh around21:02UTC. Final900000 untouched.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
| cash | 97101.7083 | 92815.2500 |
| margin | 1314.4444 | 6590.8438 |
| discards | 9.0417 | 1.2031 |
| buy_WHEAT | 351.8194 | 169.4688 |
| sell_WHEAT | 553.8333 | 311.5938 |
| net_WHEAT | 202.0139 | 142.1250 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 110.3472 | 81.9531 |
| net_CARROT | 110.3472 | 81.9531 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 34.6250 | 2.2500 |
| net_TOMATO | 34.6250 | 2.2500 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 221.0694 | 249.3906 |
| net_STRAWBERRY | 221.0694 | 249.3906 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 78.4306 | 72.0000 |
| net_MELON | 78.4306 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 57.1389 | 65.0000 |
| net_EGG | 57.1389 | 65.0000 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 191.8889 | 230.1875 |
| net_MILK | 191.8889 | 230.1875 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 147.4861 | 176.6875 |
| net_WOOL | 147.4861 | 176.6875 |
| buy_FERTILIZER | 54.9861 | 50.2344 |
| sell_FERTILIZER | 290.3750 | 357.4375 |
| net_FERTILIZER | 235.3889 | 307.2031 |
| animal_days_COW | 185.6806 | 198.9375 |
| fed_COW | 0.7934 | 0.8861 |
| cared_COW | 0.8197 | 0.9519 |
| collected_fertilizer_COW | 0.9448 | 0.9519 |
| COW_d0 | 2.2500 | 2.0000 |
| COW_d2 | 2.9028 | 3.0000 |
| COW_d5 | 3.8611 | 4.0000 |
| COW_d8 | 6.6667 | 7.5625 |
| COW_d11 | 7.2222 | 7.5625 |
| COW_d17 | 7.3611 | 7.5625 |
| COW_d23 | 7.0000 | 7.5625 |
| COW_d29 | 6.3333 | 7.5625 |
| animal_days_SHEEP | 152.3750 | 159.8125 |
| fed_SHEEP | 0.8183 | 0.9335 |
| cared_SHEEP | 0.7882 | 0.9647 |
| collected_fertilizer_SHEEP | 0.9208 | 0.8893 |
| SHEEP_d0 | 2.0000 | 2.0000 |
| SHEEP_d2 | 2.0000 | 2.0000 |
| SHEEP_d5 | 2.2083 | 2.0000 |
| SHEEP_d8 | 4.3056 | 4.4375 |
| SHEEP_d11 | 6.1389 | 6.6875 |
| SHEEP_d17 | 6.7500 | 6.6875 |
| SHEEP_d23 | 6.3611 | 6.6875 |
| SHEEP_d29 | 4.6389 | 6.6875 |
| animal_days_GOOSE | 38.0556 | 54.2500 |
| fed_GOOSE | 0.8970 | 0.8987 |
| cared_GOOSE | 0.8803 | 0.9683 |
| collected_fertilizer_GOOSE | 0.9049 | 0.8037 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1250 | 0.0000 |
| GOOSE_d8 | 0.7083 | 0.0000 |
| GOOSE_d11 | 1.5972 | 2.7500 |
| GOOSE_d17 | 1.7500 | 2.7500 |
| GOOSE_d23 | 1.7917 | 2.7500 |
| GOOSE_d29 | 1.7917 | 2.7500 |
| crop_days | 1505.3472 | 1506.8750 |
| crop_water_rate | 0.7300 | 0.7221 |
| crop_yield_day_maximized | 0.3719 | 0.2095 |
| harvest_WHEAT | 519.5417 | 516.1719 |
| harvest_CARROT | 111.7083 | 81.9531 |
| harvest_TOMATO | 34.8194 | 2.2500 |
| harvest_STRAWBERRY | 222.1667 | 249.3906 |
| harvest_MELON | 78.4306 | 72.0000 |
| hires | 279.7917 | 260.3125 |
| hire_cost | 5452.7361 | 3727.1875 |
| land_buys | 2.0417 | 2.0000 |
| land_cost | 3166.6667 | 3000.0000 |
| weed_digs | 20.6806 | 20.2969 |
| unit_faults | 72.0139 | 10.6406 |
| SELL_weighted_hour | 8.3158 | 10.3346 |
| BUY_PRODUCT_weighted_hour | 6.3448 | 5.8110 |
