# Traces

| File | Content |
|---|---|
| `cpp_games/forecast_vs_wages_1600_seat{0,1}.trace.gz` | mirror loss of `forecast` vs `wages` (seed 1600: -6,098 / -7,105); seat 0 destroyed 54 units on day 26 (WEAKNESSES 3) |
| `cpp_games/forecast_vs_king_rc4_701_seat{0,1}.trace.gz` | a close win vs king_rc4 (+6,053 / +5,805) |
| `cpp_games/*_seat0_dawns.txt` | per dawn for our seat: cash (own/opponent), land, plants and animals of both farms, compile status, fallback, compile ms, decoded intent, failed attempts |
| `cpp_games/results.csv` | full_games rows of those games |
| `local_lb_forecast_roster10.jsonl.gz` | `forecast`'s exact Local-LB replay (140 games vs the 10-agent roster of 490c63d); one JSON per line with money, timing and a per-dawn record: both farms, the compiled plan line (status, hires, returns, intent summary, every failed attempt), shops, prices |
| `frozen_top10_145.tar.gz` | the 145 frozen top-10 games (`data/traces/<episode>.txt`), for `replay_games data/replay_opponents.txt` |
| `kaggle/robust_56553038_traces.tar.gz` | all episodes of our Kaggle submission `robust` (64 public + the validation game) as engine traces (`data/kaggle_traces/`) |
| `kaggle/meta_56553038.csv` | per episode: our seat, opponent team and submission, rewards, time |
| `kaggle/frozen_kaggle_list.txt` | the 64 public games as a frozen bed (replayed seat = the opponent) |

Engine traces (`fast_game_engine/export_trace.py` format, read by `load_replay` in
`experiment/source/world.hpp`): `<seed> 719`, `CONFIG <11 config values>`, `ENGINE <version> <sha>`,
one line per seat per turn (`n_units n_orders`, then `op arg n` per unit and `op item n` per order),
then `TRUTH` with both players' money, market inventory and a state hash per step. They replay bit-exactly in our engine:
`build/trace_check <trace>` verifies, `build/replay_games` replays one seat against our agent,
`analysis/` tools (`ledger`, `melon_audit`, `market_audit`, `trip_audit`, ...) take lists of traces.

Regenerate or extend: `BC_WRITE_TRACES=<dir> BC_GAME_TRACE=<seed>:<seat> build/full_games <opponent>
<seed> <n> <threads> out.csv` (opponent `bc:<model.bin>` for our own candidates).
