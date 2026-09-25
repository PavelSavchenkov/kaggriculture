# Candidates: sidecar sets of every lineage agent

All candidates use the same network (`../weights/v12_cond/model.bin`, sha256 `4ef6b5fb...`). A
candidate folder = that `model.bin` + these sidecars (`../setup_experiment.sh` creates
`models/<name>/` with the weights hard-linked). Sidecar formats: ARCHITECTURE.md section 5.

| Name | Local-LB id / model folder | Opening | Compiler sidecar | Decode sidecar | Status |
|---|---|---|---|---|---|
| `vadim` | pavel-bc-opus-v12-vadim / cand_v12_vadim6 | Vadim `6 8` | `reserve_from_day 0`, `recovery 0` + env `DC10_TRIM_D0_ANIMALS=1` (file `ENV`) | - | Local-LB PR 1, 333/400 |
| `herd` | pavel-bc-opus-v12-herd | Vadim | `recovery 0` | - | Local-LB #1 after merge |
| `robust` | pavel-bc-opus-v12-robust | Vadim | `reserve_from_day 0`, `recovery 0` | - | Kaggle submission 56553038 |
| `slots` | pavel-bc-opus-v12-slots / cand_so2 | Vadim | `sell_order 2` | - | Local-LB #1, 399-1-0 |
| `wages` | pavel-bc-opus-v12-wages / cand_so2rw | Vadim | + `return_wages 1` | - | merged |
| `forecast` | pavel-bc-opus-v12-forecast / fin_I | DSM `6 7` | + `hire_cap_stock 1`, `collect_all 1`, `forecast_blend 1` | herd reach days 6-14, q 0.8 0.7 0.6 | **best; pushed** (= `../agent/model`) |
| `select` | fin_T | DSM | forecast + `forecast_select 1` | as forecast | C++ panel +658 [+126, +1,315] vs forecast |
| `routes` | fin_P (package `sep25-bc-opus-v12-routes`, not pushed) | DSM | forecast + `blend_visible 70`, `race_dp 1`, `route_search 2` | as forecast | 89-71 vs forecast |
| `anticipate` | fin_Y (package `sep26-bc-opus-v12-anticipate`, not pushed) | DSM | select + `forecast_prior`, `race_dp`, `race_early`, `race_steps 1`, `melon_tie_now` | as forecast | mirror 36-4, 29-11 (three herd-reach collapses); C++ panel +1,549 [-193, +3,160] |
| **`z_rs`** | fin_Z_rs (not packaged) | DSM | anticipate without `forecast_prior` | forecast's + `reach_stress 1` | **strongest at the snapshot**: one-seat 19-1, 18-2 vs forecast; 37-23 vs Y+rs; C++ panel +1,382, 222/224; needs Local-LB confirmation |

The first five were built when their rules were the compiler defaults; with the snapshot's code the
sidecars above reproduce their recorded C++ games bit-exactly (checked on seed 700-701 vs king_rc4 for
herd, robust, slots, wages and vadim). The Local-LB repository keeps each shipped package
(`agents/pavel-bc-opus-v12-*`, with its own bridge and sources).
