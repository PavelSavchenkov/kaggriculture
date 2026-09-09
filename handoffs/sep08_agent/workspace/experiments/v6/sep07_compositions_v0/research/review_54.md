# Review54 — 2026-09-07T19:50:21.377381+00:00

Due19:48UTC. The original goal remains active through September8,00:47UTC.
Current reference remains opening_q32_b13_v1. No Git or new submission.

The user's day-solver suggestion worked. More exact search removes8/$898
extra hires in each goose continuation. On now has no extra hires; off still
has one $89 hire on day22. Full-game sales remain unchanged. Final-day work
leaves one unsold fertilizer uncollected. Cow/sheep received the same30-second
retry: all extra hires disappear for sheep and the on cow course, with exact
production/sales/rival-action preservation in64games per leaf. Off cow still
has three unresolved days. UNKNOWN remains a search result, not a minimum.

The first complete goose policy passed a3328-game discovery screen but failed
the43008-game fresh broad screen. Its wrapper had replaced the parent's
tomato-selected future despite matching the same physical entry state. In181
of1024 direct games it lost24tomatoes and added wheat/goose, averaging-$1005.48
margin change. The intended392 wheat-to-goose games averaged+$199.11. This
exposes a compilation-context error, not an error in the predicted28eggs.
The small crop-control loss was an early signal we should have investigated.

late_goose_wheat_context fixes this by preserving the parent's existing
day12 two-tomato-shop rule. On a second, newly preregistered43008-game panel,
it wins420, ties480 and loses124 direct parent games, mean margin+$80.35.
Every opponent's paired mean margin improves. However current grouped utility
falls.9454834 to.9407837, with95% gain[-.0076904,-.0020142]; historical and
individual utility gates also fail. No promotion. The corrected context removes
the large unintended losses; an economic investment rule is still needed.

Next select among complete goose/cow/sheep calendars and retaining crops using
the existing fast whole-farm observed-market estimator. Use their actual
compiled daily sales/buys, fixed seed/animal costs, and achievable hire costs.
Separate the already-used1790000/1810000 panels from new validation. Branch on
future berry demand only when those shops are observed, and compare conservative
or sampled expected continuation values at day13. Preserve intent as well as
physical compatibility when importing any new calendar. This is a direct
instance of the original estimate/compile/measure feedback loop.

New public notebook audit is complete: V5 Hybrid repeats the already ported
five tapes/trees and adds a small weed-clearing rule. The C++ diagnostic port
has896 discovery games, unchanged win counts, active-opponent margin gains0
to+$6.69 and PASS loss-$4.86. It is not a new strong family. Fields of Fortune
has geographical crop zones and greedy placement but explicitly disables
animals; its valuations omit important biology/service costs. Can Specialists
Beat One Agent is presentation material without an executable policy. Exact
hashes, comparison and decisions are in research/refresh_1902/NOTEBOOK_AUDIT.md.

Keep the full original goal active: dated composition search, fast economics
and service feasibility, placement/worker/trade optimization, general animal
and wait choices, larger/cold proposals, reusable top-player components, and
growing-league validation. Neither good schedules nor positive average cash
alone is enough to promote a policy that loses league win probability.

The82 comparisons below are unchanged19:02 global72 and promoted-q32 local64.
No new promoted policy or global replay cohort exists. Next review20:08UTC;
refresh public sources around20:02UTC. The final900000 seed reserve is unused.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
| cash | 97672.4583 | 92815.2500 |
| margin | 637.1944 | 6590.8438 |
| discards | 7.1528 | 1.2031 |
| buy_WHEAT | 346.9028 | 169.4688 |
| sell_WHEAT | 549.7917 | 311.5938 |
| net_WHEAT | 202.8889 | 142.1250 |
| buy_CARROT | 0.0000 | 0.0000 |
| sell_CARROT | 128.6806 | 81.9531 |
| net_CARROT | 128.6806 | 81.9531 |
| buy_TOMATO | 0.0000 | 0.0000 |
| sell_TOMATO | 33.5833 | 2.2500 |
| net_TOMATO | 33.5833 | 2.2500 |
| buy_STRAWBERRY | 0.0000 | 0.0000 |
| sell_STRAWBERRY | 229.2361 | 249.3906 |
| net_STRAWBERRY | 229.2361 | 249.3906 |
| buy_MELON | 0.0000 | 0.0000 |
| sell_MELON | 77.4861 | 72.0000 |
| net_MELON | 77.4861 | 72.0000 |
| buy_EGG | 0.0000 | 0.0000 |
| sell_EGG | 61.6806 | 65.0000 |
| net_EGG | 61.6806 | 65.0000 |
| buy_MILK | 0.0000 | 0.0000 |
| sell_MILK | 183.9861 | 230.1875 |
| net_MILK | 183.9861 | 230.1875 |
| buy_WOOL | 0.0000 | 0.0000 |
| sell_WOOL | 138.4583 | 176.6875 |
| net_WOOL | 138.4583 | 176.6875 |
| buy_FERTILIZER | 56.5694 | 50.2344 |
| sell_FERTILIZER | 273.0417 | 357.4375 |
| net_FERTILIZER | 216.4722 | 307.2031 |
| animal_days_COW | 181.3750 | 198.9375 |
| fed_COW | 0.7773 | 0.8861 |
| cared_COW | 0.8057 | 0.9519 |
| collected_fertilizer_COW | 0.9419 | 0.9519 |
| COW_d0 | 2.2500 | 2.0000 |
| COW_d2 | 2.9028 | 3.0000 |
| COW_d5 | 3.8333 | 4.0000 |
| COW_d8 | 6.8750 | 7.5625 |
| COW_d11 | 7.3472 | 7.5625 |
| COW_d17 | 7.4028 | 7.5625 |
| COW_d23 | 6.6944 | 7.5625 |
| COW_d29 | 5.8194 | 7.5625 |
| animal_days_SHEEP | 144.6944 | 159.8125 |
| fed_SHEEP | 0.8070 | 0.9335 |
| cared_SHEEP | 0.7758 | 0.9647 |
| collected_fertilizer_SHEEP | 0.9268 | 0.8893 |
| SHEEP_d0 | 2.0000 | 2.0000 |
| SHEEP_d2 | 2.0000 | 2.0000 |
| SHEEP_d5 | 2.2639 | 2.0000 |
| SHEEP_d8 | 4.1944 | 4.4375 |
| SHEEP_d11 | 5.9444 | 6.6875 |
| SHEEP_d17 | 6.4444 | 6.6875 |
| SHEEP_d23 | 5.9861 | 6.6875 |
| SHEEP_d29 | 3.8889 | 6.6875 |
| animal_days_GOOSE | 40.2222 | 54.2500 |
| fed_GOOSE | 0.9056 | 0.8987 |
| cared_GOOSE | 0.8714 | 0.9683 |
| collected_fertilizer_GOOSE | 0.9084 | 0.8037 |
| GOOSE_d0 | 0.0833 | 0.0000 |
| GOOSE_d2 | 0.0833 | 0.0000 |
| GOOSE_d5 | 0.1528 | 0.0000 |
| GOOSE_d8 | 0.8472 | 0.0000 |
| GOOSE_d11 | 1.7222 | 2.7500 |
| GOOSE_d17 | 1.8333 | 2.7500 |
| GOOSE_d23 | 1.8611 | 2.7500 |
| GOOSE_d29 | 1.8611 | 2.7500 |
| crop_days | 1506.7778 | 1506.8750 |
| crop_water_rate | 0.7272 | 0.7221 |
| crop_yield_day_maximized | 0.4180 | 0.2095 |
| harvest_WHEAT | 504.5694 | 516.1719 |
| harvest_CARROT | 130.0278 | 81.9531 |
| harvest_TOMATO | 33.7917 | 2.2500 |
| harvest_STRAWBERRY | 230.1250 | 249.3906 |
| harvest_MELON | 77.6806 | 72.0000 |
| hires | 276.9167 | 260.3125 |
| hire_cost | 5268.8056 | 3727.1875 |
| land_buys | 1.9861 | 2.0000 |
| land_cost | 3000.0000 | 3000.0000 |
| weed_digs | 20.6944 | 20.2969 |
| unit_faults | 57.1667 | 10.6406 |
| SELL_weighted_hour | 7.9508 | 10.3346 |
| BUY_PRODUCT_weighted_hour | 6.8210 | 5.8110 |
