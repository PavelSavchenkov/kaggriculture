# Results

All numbers are margins (own final money minus opponent's) from our agent's side. "Wins" count
games with a positive margin. Seats of one seed often mirror each other, so 40 games (20 seeds x 2
seats) carry about 20-30 independent outcomes. Raw per-game data for every row: `evidence/`
(`summary.csv` has one line per run).

## 1. Local-LB (team league, official engine, Python harness)

The Local-LB ([T3pp31/kaggriculture-localLB](https://github.com/T3pp31/kaggriculture-localLB)) plays
a challenger against every active agent on fixed seeds. We replay those exact games locally
(`scripts/lb_seeds_roster.sh`, older rules `lb_seeds*.sh`); the prediction matched the table after
every merge (e.g. `slots`: predicted 399/400, ranked 399-1-0).

### Our agents on the Local-LB

| Agent | Local-LB result (at the time) | Exact local replay of its Local-LB games |
|---|---|---|
| `pavel-bc-opus-v12-vadim` | 333/400, Elo 1892 (PR 1); later #3-#4 | 333/400, +$9.7k (`lbseeds/base`) |
| `pavel-bc-opus-v12-herd` | #1, Elo 2283 (394-6) after its merge | 398/400, +$14.5k (`lbseeds/herd`) |
| `pavel-bc-opus-v12-robust` | #2, Elo 1932-2098 | 367/400 on its own evaluation set (`lbseeds2/robust`) |
| `pavel-bc-opus-v12-slots` | **#1, 399-1-0, rating 2802** (Sep 25 16:20) | 399/400, +$13.1k (`lbseeds3/so2`) |
| `pavel-bc-opus-v12-wages` | merged (PR #184), ranking rebuild was pending | 386/400 vs the df4172a roster; 28/40 vs `slots` (`lbseeds4/so2rw`) |
| `pavel-bc-opus-v12-forecast` | pushed Sep 25 17:45 (branch `submit/pavel-bc-opus-v12-forecast`) | 137/140, +$13.8k vs the 10-agent roster of 490c63d (`roster10/fin_I`) |

Table snapshot of Sep 25 15:15 UTC (before `wages`/`forecast`): slots 2802, herd 2088, robust 1932,
vadim 1638, ttyn master engine v3 1496, wzhengbiao v15stack 1279, arsgorynich herd-safe v3 1264,
shiiin9 order book 1102, ahmed productive wheat v54 996, cha22 route replay 959.

### `forecast` against every agent in the Local-LB repository

966 games (69 agents x 7 seeds x 2 seats, Sep 26 seed rule, `evidence/python_harness.csv` set
`roster_all/fin_I`): 962 wins, mean +$37.5k. Losses: `slots` 12-2 (+$3.5k mean), `wages` 13-1
(+$3.4k), `ppo-v6n-it60` 13-1 (+$15.1k). Against third-party agents it never lost.

### Broad unseen-seed panel (all 10 Local-LB agents of the time, seeds 1300-1323, both seats)

| Compiler / setting | Games | Wins | Mean margin |
|---|---|---|---|
| base = `vadim` | 480 | 86.0% | +$9.9k |
| X1 = `herd` rules (selection seeds 1300-1311) | 240 | 100% | +$14.8k |
| X1, holdout seeds 1312-1323 | 240 | 95.0% | +$18.1k |
| trim0 = `robust` rules | 240 | 100% | +$14.2k |
| sp (herd rules + recovery + sales by price) | 240 | 100% | +$15.0k |
| `wages` (so2rw) | 120 | 100% | +$20.4k |
| v16 network (replay-DB data) + trim0 | 120 | 91.7% | +$7.5k |

## 2. C++ mirror (our candidates head to head)

`tools/search_games` without search, both agents with their own sidecars, 20 seeds x 2 seats per set.

### `forecast` (fin_I) vs `wages` (so2rw)

| Seeds | 1300 | 1400 | 1500 | 1600 | 7000 (Local-LB seeds of the pairing) | Total |
|---|---|---|---|---|---|---|
| W-L | 33-7 | 34-6 | 34-6 | 28-12 | 35-5 | **164-36** |
| Mean margin | +3,409 | +4,320 | +3,335 | +1,921 | +3,123 | +3.2k |

How the parts added up (each row vs the row above unless noted, per seed set):

| Candidate | Change | Result |
|---|---|---|
| fin_C | DSM opening + hire cap counts shed stock (vs `wages`) | 26-14, 31-9, 26-14, 23-17, 15-25 = 121-79 |
| fin_D | + herd reach days 6-14 (vs `wages`) | 28-12, 31-9, 29-11, 20-20, 18-22 = 126-74 |
| fin_G | fin_D + collect all fertilizer (vs fin_D) | 28-12, 29-11, 31-9 = 88-32 |
| fin_H | fin_D + visible-supply forecast blend (vs fin_D) | 35-5, 30-10, 36-4 = 101-19 |
| fin_I = `forecast` | fin_D + both (vs `wages`) | 164-36 (above) |

### After `forecast`: candidates vs fin_I

| Candidate | Change on top of fin_I | Mirror vs fin_I | C++ panel vs fin_I (224 games, paired, seed-clustered 95% CI) |
|---|---|---|---|
| **`z_rs` (fin_Z_rs)** | `anticipate` without the hourly prior + `reach_stress 1` (herd reach must also be funded under the stress forecast) | one seat: 19-1 (+6.6k), 18-2 (+4.0k); head to head vs Y+rs 37-23 | +1,382 [-554, +3,315], 222/224 wins |
| fin_Y_rs | `anticipate` + `reach_stress 1` | one seat: 19-1 (+2.8k), 18-2 (+2.6k) | +1,890 [+226, +3,685] |
| fin_Z | `anticipate` without the hourly prior | one seat: 19-1 (+3.8k), 19-1 (+2.0k) | not run |
| fin_I_rs | `forecast` + `reach_stress 1` | one seat: 6-5-9, 7-6-7 (neutral: changes only fragile days) | not run |
| fin_Z_sd | Z + `stress_down` (drop to a stress-funded smaller herd) | one seat: 17-3, 15-5 | dropped |
| 4th-quadrant probes | `land_push` / a Q4 fine-tune as `land_model` on top of Z+rs | one seat 1300 vs fin_I: 8-12 to 18-2 (+0.2k to +5.1k), all below Z+rs's 19-1 (+6.6k) | rejected for now (shed overflow; LINEAGE 4) |
| `anticipate` (fin_Y) | online forecast selection + anticipation stack (hourly prior, DP delivery hours from hour 6, retries, melons sold on arrival) | 36-4 (+2.3k), 29-11 (-0.7k: three collapses, see LINEAGE 4) | +1,549 [-193, +3,160] |
| `select` (fin_T) | per-product choice of blend vs fitted forecast by recent error | 17-23, 28-12 | +658 [+126, +1,315] |
| fin_S | fitted linear opponent forecast | 18-22, 20-20, 17-23 | +1,209 [-194, +2,575] |
| fin_W | anticipation stack alone | 32-8, 27-13 | +916 [-1,561, +3,206] (160 games) |
| `routes` (fin_P) | blend 0.7 + race_dp + wide route search on >= 12-hire days | 24-16, 31-9, 19-21, 15-25 = 89-71 | not run |
| fin_L | race_dp | 86-64-10 over 4 sets (+0.3k, +50% compile time) | not run |
| fin_U / fin_V | forecast conditioned on the opponent's sales so far today | 49-27-4 / 46-34 | -69 / +157 (dropped) |
| fin_J | wide route search on every solve | 73-47 (+0.6k) | too slow: 73 s compile per game |
| fin_K, fin_M, fin_N, fin_Iv | slack care / melon race / crop reach / Vadim opening | 9-31 / 41-39 / 60-60 / 13-27 | rejected |

"One seat" = `scripts/mirror1.sh` (seat 0 only, 20 games per set): the two seats of a mirror seed
mostly repeat one game, so this halves the CPU for the same information. The mirror cannot show
forecast gains: against our own lineage the opponent's sales are already well predicted. Forecast
variants must be judged against varied opponents (C++ panel, Local-LB). Mirror means are dominated by
rare collapses (a game lost by $10-30k); count wins too.

## 3. C++ opponents (full_games, seeds 700-715, both seats)

`forecast`: 224/224. Mean margin per opponent: agent_sep23 +30.5k, ahmed_v25 +33.2k, arlene_v4_m31
+28.2k, investment_context_guarded_001_best +25.9k, king_rc4 +26.9k, teammate_shoprouter +30.9k,
two_random_shop_league_v179 +73.4k (`evidence/cpp_panels.csv`, set `panel/cpp_fin_I`).

## 4. Frozen top-team replays (145 held-out games of the Kaggle top 10, Sep 18+)

We replace one seat; the other repeats its recorded orders (it does not react, so these opponents are
weaker than the originals). `forecast` 143/145 (+$54.4k), herd reach days 8-14 139/145, `wages`
136/145 (+$47.7k), `slots` 135/145, DSM opening 134/145, `robust` 132/145 (0 collapses), `herd`
130/145 (3 farm collapses), `vadim` 127/145.

## 5. Zoo league (network quality; both sides use the same compiler)

Six networks fine-tuned on single top teams (DSM, DECEM, Mother-Goose, Majkel, Vadim, M&M) vs our
network, C++ games, seeds 1300-1315 both seats (192 games): v12 with herd rules 138/192; with
`slots` 190/192; with `wages` 192/192; v16 91/192; v18 97/192 (`evidence/cpp_panels.csv`, `zoo/*`).

## 6. Kaggle

Only `robust` (submission 56553038, Sep 25 14:32 UTC) was submitted. Public score 2075.9 after 18
games, 2696 after 28, 2795.1 after 64 games (50 wins) at 19:35 BST; the team's best score. All 14
losses were against top-50 players (ranks 5-49). Per-loss ledger and causes: WEAKNESSES.md and
`docs/weakness_analysis/KAGGLE_REPLAYS.md`; the 64 games are in `traces/kaggle/`. Every later agent
(`slots`, `wages`, `forecast`) fixes some of those loss causes but has not been submitted.

## 7. Network screens (C++ mirror under the `wages` compiler, vs v12, 40 games)

| Network | Wins of 40 | Note |
|---|---|---|
| v12_aug (dihedral grid augmentation) | 18 | |
| v13_w384b (width 384) | 14 | lower validation loss (16.5 vs 17.2) |
| v14_all (w384b + 17.7k more perspectives) | 15 | |
| v16_db (replay-DB data, 80k steps) | 7 | |
| v18_dbstrong (strong replay-DB part) | 4 | |
| v12 recipe, seeds 1-11 | 8, 19, 5, 20, 16, 9, 3, 7, 11, 23, 12 | v12 is a top-decile draw |

Validation loss does not predict game strength across these runs; select networks by games.

## 8. Runtime

Worst compile per dawn about 2.4-4.5 s on a loaded machine (C++ panel `compile_ms_max`); at most 0.7 s
of the 60 s overage used per Local-LB game; Kaggle validation of `robust` left 56.8 s. A valve in the
bridge switches to faster search below 25 s of remaining overage and to capacity-only returns below
10 s; route search and race_dp switch off below 40 s and 30 s.
