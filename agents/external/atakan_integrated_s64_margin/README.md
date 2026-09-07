# Atakan future-shop integration

Retain `proposals/atakan_integrated_s64_margin` as the improved margin specialist. It averages the original three-course economic forecast over 64 legal future-shop scenarios, then chooses the highest estimated own-minus-rival profit. Keep the earlier demand specialist. Neither beats our current main agent. The full-mean variant remains a control: it looked strongest on discovery but lost that advantage on fresh seeds.

The only policy change is unknown-shop integration. Mode0 reproduces the original 35% expected-demand discount; mode1 uses the undiscounted mean; modes8,32,64 integrate deterministic stratified future-shop sequences. Every block of eight samples contains every shop once at each unknown reveal, and all three branches share the same scenarios. The sampler sees no environment seed, future observation or rival-private state. Current revealed shops, own state and shared prices still condition the choice.

The three complete replay courses, recorded own daily trade quantities, fixed costs, rival current-herd forecast, current-day fraction, daily midpoint prices, internal netting and price-floor handling are unchanged. The baseline's town-center demand approximation is also unchanged so this experiment isolates integration. The result still assumes donor logistics and a simplified rival future service plan. Courses come from Atakan replay programs156/160/157; exact replay hashes and the copied root sampling design hash are in `LINEAGE.json` and every `IMPORT.json`.

Discovery used the existing 320 contexts plus 64 against `investment_context_guarded_001_best`. The following rows cover the original five-opponent panel:

| Objective and integration | Wins / 320 | Mean margin | Mean branch regret |
| --- | ---: | ---: | ---: |
| Own, original35% | 211 | $3,347 | $3,348 |
| Own, full mean | 213 | $3,366 | $3,329 |
| Own, samples8/32/64 | 213 | $3,408 | $3,287 |
| Margin, original35% | 211 | $3,446 | $3,249 |
| Margin, full mean | 226 | $4,910 | $1,785 |
| Margin, samples8 | 222 | $4,531 | $2,164 |
| Margin, samples32 | 222 | $4,578 | $2,118 |
| Margin, samples64 | 222 | $4,608 | $2,087 |

All 3,840 new-policy discovery games match the complete fixed-course outcome selected by the diagnostic, including both action hashes and cash. This validates the forecast-choice evaluation against full realization. Sampled own-profit choices are identical for8/32/64 on these384 contexts; this is measured behavior, not proof of global policy equivalence. Legacy mode0 reproduces all1,152 original three-branch forecasts exactly.

Fresh audit used unused seeds1450000–1450255 in both seats against six opponents, including the current main reference. Every policy below has3,072 games on the same scenarios:

| Policy | Wins | Mean margin | Win change vs old margin | Margin change vs old margin |
| --- | ---: | ---: | ---: | ---: |
| Old margin selector | 1,981 | $3,749 | — | — |
| Existing demand selector | 2,058 | $4,304 | +77 | +$555 |
| Full-mean margin | 1,974 | $4,115 | −7 | +$366 |
| Sampled64 margin | 2,028 | $4,407 | +47 | +$658 |

For sampled64 versus old margin, the paired bootstrap95% interval is +$270 to+$1,080 for mean margin gain and +0.07 to+3.06 percentage points for win-rate gain. Resampling keeps both seats and all six opponents together within each of256 seed clusters. These descriptive intervals reflect this audit distribution, not future leaderboard performance.

Sampled64 gains against public router V5 (295/512 wins versus263), teammate (414 versus410), and King (480 versus478). It loses five wins against the current main reference (221 versus226), while improving mean margin from−$1,337 to−$1,078. Relative to demand, sampled64 has30 fewer wins overall and+$103 mean margin; both bootstrap intervals include zero. Keep both specialists rather than asserting that one dominates the other.

Direct512-game matches reinforce the limited conclusion. Sampled64 versus old margin is68 wins,400 ties,44 losses and+$500 mean margin. Against demand it is67 wins,374 ties,71 losses and+$234 mean margin. Full mean versus sampled64 is40 wins,416 ties,56 losses and−$340 mean margin.

The official native shop-RNG check uses256 games per opponent on seeds1450000–1450127. Sampled64 wins97 against the current main reference,140 against V5 and193 against teammate; old margin wins100,122 and188, respectively. The native result confirms the V5/teammate improvement and the remaining main-agent weakness. This is exact local C++ validation, not a packed official-runtime or submission claim.

All retained-policy checks pass: PASS256, self8, sixteen exact generic-versus-mask-checking/thread games, and sixteen typed-debug-versus-generic games. A further13,122 equalities verify that the stratified average demand equals the full mean while retaining the same current-day factor. Average time for the entire three-course decision is4.0µs for the mean,35.2µs for8 samples,142.4µs for32 and292.0µs for64. The sampled64 median is286.6µs and95th percentile310.5µs across384 observed contexts, including sample generation; concurrent system work can affect timings.

`REPORT.json` contains complete fresh, direct, native and operational results. `DIAGNOSTIC_REPORT.json` and `decision_cases.json` retain choice regret and runtime evidence. `SOURCE_REALIZATION_PARITY.json` documents complete-course identity. `build/*/BUILD.json`, `*_COMMANDS.json` and `FINAL_VALIDATION.json` freeze source/dependency, binary, command and artifact hashes. Existing Atakan policies and the oracle report were not changed.
