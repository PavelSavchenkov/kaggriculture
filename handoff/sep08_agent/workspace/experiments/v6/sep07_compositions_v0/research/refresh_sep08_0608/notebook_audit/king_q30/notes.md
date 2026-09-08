# Kaggriculture 2312.9 — Q30 v2a Replay-Programme Agent

> **Leaderboard note:** when this notebook was prepared, this exact `q30_v2a` submission was shown at **2312.9** on the Kaggriculture submissions page, with the result displayed as **19 hours ago**. Leaderboard values can change as evaluation continues.

This notebook open-sources the complete reviewed `q30_v2a` submission and explains the control logic around it. The source below is preserved **verbatim** from the submitted `main.py`; the notebook only splits it into readable sections and packages it back into a Kaggle-ready `.tar.gz`.

**What kind of agent is this?** It is not a primitive-action RL policy. It is a replay-distilled **programme network**: physical worker choreography is selected from 72-turn programme blocks, while market actions are retrieved/adapted from replay-derived state-action examples and then corrected by a small number of closed-loop overlays.

## 1. Effective runtime architecture

The file contains several generations of experiments for provenance, but the **final effective path** is much simpler than the source order suggests:

```text
observation
   │
   ▼
q30_v2a
   │
   └── generic_q30_v1
         │
         ├── _dusta_terminal_v2
         │      └── frozen Hybrid opening/base programme
         │             └── OceanMix programme network
         │                    ├── shop-conditioned 72-turn physical choreography
         │                    └── nearest-state MARKET_LIB market action
         │
         ├── _v2_structural_recovery   # weed-blocked BUILD transaction repair
         ├── q30 step-0/1 Wheat probe  # buy 30, then normally sell surplus 25
         └── LOW-response affordability cap
   │
   └── v2a monetization overlay
          ├── shop-conditioned small inventory sales
          └── opponent visible-crop maturity / supply forecast
```

A useful reading rule: later `agent = ...` assignments do **not** imply that every historical controller is stacked into the final policy. `generic_q30_v1` explicitly calls `_dusta_terminal_v2`, and `q30_v2a` explicitly wraps `generic_q30_v1`.

## 2. Core replay-programme idea

The base layer was reconstructed from **31 OceanMix trajectories / 30 unique episodes** (as documented in the source comments).

Two complementary mechanisms are used:

- **Physical programme routing.** `PROGRAM` contains strategy families such as `BAL`, `MILK`, `PET`, and `YARN_*`, broken into mostly 72-turn blocks. The unlocked-shop prefix selects the current family.
- **Nearest-state market overlay.** `MARKET_LIB` stores market decisions indexed by turn and programme family. `_feat()` builds a 43-dimensional public/private state vector (market inventory, shed inventory, seeds, money, animals, crops, shops), and `_dist()` performs a hand-weighted normalized L1 nearest-neighbour lookup.
- **Safety sanitation.** `_safe()` only fixes action-shape mismatches (e.g. number of hired hands); it intentionally does not replace the replay choreography with a generic planner.

This is why the agent is best understood as **retrieval + programme execution + sparse feedback correction**, rather than conventional reinforcement learning.

### Source A — Core programme network

The two large compressed constants are the replay-distilled programme and market libraries. The executable logic after them defines shop-family routing, the state feature vector, weighted nearest-neighbour market retrieval, and basic action-shape safety.

### Source B — Opening-capital guard

At turn 1 the agent simulates the visible cost of the opening basket. If HIRE / animal purchases are at risk, it moves structural purchases ahead of lower-priority product buys. The intention is to preserve the physical programme rather than let a market-price shock silently break the opening.

### Source C — Reversible Wheat probe

A small early Wheat position is used as a reversible market probe. The baseline version buys 6 Wheat at step 0, keeps a 5-Wheat reserve, and sells only the surplus at step 1. This is an early precursor to the later q30 probe.

### Source D — Dusta terminal logistics v1

This layer detects the late-game deadline and recalls product-carrying workers toward the shed in time to `DROP`, then liquidates shed inventory near the end of the game.

### Source E — Dusta terminal logistics v2

The reviewed v2 keeps the expert market sequence intact and **appends only uncovered liquidation quantities**. Carriers are recalled at their latest safe departure time, minimizing disruption to the productive programme.

### Source F — Historical sparse super-agent experiments

These blocks are retained in the reviewed source for provenance. They contain public-state opponent classification and some programme-route repairs. **They are not on the final q30_v2a effective call path.**

### Source G — Closed-loop structural recovery

This is one of the important layers still reused by q30. When a `BUILD_PASTURE` / `BUILD_COOP` would deterministically fail because the actor is standing on a weed, the controller executes `DIG`, queues the displaced BUILD, and replays delayed non-PASS work until natural PASS slack catches up. The queue is reset at day boundaries.

### Source H — Historical normalization / capital-control experiments

This region records iterations such as SELL-front-running, step normalization, duplicate-SELL merging, a small opening-q controller, and Capital-Adaptive Aegis. They help document the research path but are bypassed by the final q30_v2a base.

### Source I — Historical Anti-Router synchronization

This section contains an exact-early-synchronization / shadow-expert experiment. It is retained but does not belong to the final q30_v2a call graph.

### Source J — Generic q30

The q30 branch makes the early market probe much stronger: step 0 buys **30 Wheat**; step 1 estimates opponent Wheat demand from public inventory and sells the excess above the canonical 5-Wheat reserve. It also merges duplicate SELL orders and applies late-game sell ordering.

### Source K — q30 v1: LOW-response virtual-affordability cap

For a near-mirror state and a LOW observed response, v1 conservatively caps later Wheat `BUY_PRODUCT` quantities by the opponent’s estimated virtual affordability. This is designed to avoid paying into an unfavorable Wheat-price feedback loop while leaving structural orders unchanged.

### Source L — q30 v2a: demand / competitor-flow monetization

This is the **final layer**. From step 240 onward it opportunistically sells small lots when unlocked shops imply demand and the visible price is resilient. It also inspects the opponent’s public board: when Strawberry or Melon supply is visibly close to maturity, it can pre-monetize the agent’s own inventory before additional supply reaches the market.

The logic is deliberately identity-free: it uses shops, prices, shed quantities, and public crop maturity—not opponent names or hidden state.

## 3. What v2a changes over q30 v1

The final delta is intentionally small and market-focused:

1. **Demand-aware micro-liquidation**
   - `MILK`: unlocked dairy demand + shed ≥ 4 + price ≥ 80 → sell up to 4.
   - `WOOL`: Yarn Store + shed ≥ 2 + price ≥ 35 → sell up to 3 on selected turns.
   - `STRAWBERRY`: berry-demand shop + shed ≥ 3 + price ≥ 85 → sell up to 3.
   - `CARROT`: Pet Cafe / Farmers Market + shed ≥ 2 + price ≥ 30 → sell up to 3.

2. **Competitor-flow forecast**
   - Count visibly maturing opponent Strawberries and Melons from public tiles.
   - If substantial supply is about to arrive and current price is still high, sell a small lot first.

3. **Non-destructive composition**
   - `_q30_add_sale()` refuses to duplicate an item already being sold and respects the 10-order market limit.
   - `_q30_merge_sell()` merges duplicate lots while preserving the first occurrence order.

The design philosophy is conservative: improve monetization timing without rewriting the underlying physical replay programme.

## 4. Programme-library statistics

The following cell imports the generated `main.py` and reports a few structural facts. This is a sanity check, not part of the submitted agent.

Expected high-level structure for this reviewed file: **52 programme families**, **3,738 programme states**, and market-library entries covering **719 turns**. The final exported callable should be `q30_v2a`.

## 5. Build the Kaggle submission archive

Running this cell creates a single-file archive containing `main.py`. The source is compiled first so syntax errors are caught before packaging.

## 6. Notes for readers

- **Leaderboard score is empirical, not a theorem.** The 2312.9 figure is the displayed competition result for this exact reviewed submission at the time this notebook was prepared; future ratings can move.
- **Replay-derived does not mean pure imitation.** The agent preserves replay choreography but adds explicit public-state feedback around opening capital, structural failures, terminal logistics, opponent market response, and supply timing.
- **Historical code is intentionally retained.** Several intermediate controllers remain in `main.py` because this is the reviewed competition artifact. The effective final call graph above tells you which layers actually determine `q30_v2a` actions.
- **Reproducibility.** Running all cells rewrites the exact reviewed `main.py`, imports it, checks the final callable, compiles it, and produces a Kaggle-ready tarball.