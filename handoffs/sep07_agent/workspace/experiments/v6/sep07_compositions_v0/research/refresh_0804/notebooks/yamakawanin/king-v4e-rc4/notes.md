# King-v4e RC4 — Explained & Kaggle-Ready Submission

> **Frozen artifact notebook.**  
> This notebook explains the reviewed **King-v4e RC4** Kaggriculture agent and reconstructs the exact reviewed `main.py` byte-for-byte.  
> Running all cells produces **`submission.tar.gz`**, whose archive contains **only `main.py` at the archive root**, ready for the Kaggriculture submission interface.

## Artifact identity

- Final agent: `king_v4e_rc4`
- `main.py` SHA256: `26ffba5273e4432dbc4ec822d5a65b09d3e9f1473c3ce813c528812eeb45779d`
- Deterministic `submission.tar.gz` SHA256: `8a6e08f0b063f4edf0d72bf6787014eade57f868709b23249ddc9a005696d659`
- Runtime dependencies: Python standard library only (`base64`, `copy`, `json`, `types`, `zlib`)
- No filesystem, network, model-file, NumPy, Pandas, PyTorch, or external-package dependency at agent runtime.

The source embedded below is the exact source that passed the final submission review. The explanatory cells do **not** modify the policy.

## 1. What kind of agent is RC4?

RC4 is **not primitive-action RL** and it is not a neural policy. Its mature core is a replay-derived **programme network** with long, coordinated worker choreography. Later generations add a small number of closed-loop controllers around that backbone.

A useful high-level view is:

```text
Observation
    ↓
programme / shop-regime selection
    ↓
long coordinated physical choreography
    ↓
market state adaptation
    ↓
v3m: HIGHCAP + one-turn SELL debt/preemption
    ↓
v4e: liquidity-aware planning + YARN_SECOND option
    ↓
RC3: sparse marginal-capital overlays
      ├─ step 241 latent Sheep capacity
      └─ step 385 Fertilizer throttle
    ↓
RC4: SmartFarm public-state late-liquidity ordering
    ↓
{farmer, hands, market}
```

The key engineering rule is **do not casually rewrite the long worker choreography**. BUY, PICKUP, PLACE, FEED/CARE, HARVEST, DROP and SELL are tightly coupled. Most successful improvements therefore change capital allocation or transaction ordering only when the existing physical programme can support them.

## 2. v4e layer — liquidity + YARN_SECOND

`q30_v4e` keeps the mature underlying programme but adds a specific continuation when the **second unlocked shop is `YARN_STORE`** (and the first is not YARN). It also inherits the liquidity-aware controller developed in the preceding v4b/v3m stack.

The important design idea is that *cash on hand is not the same as free cash*. Future HIRE / seed / animal / land transactions implied by the programme consume working capital, so the agent avoids locally affordable actions that would break the next part of the production chain.

Conceptually:

```python
if step == 144 and second_shop == "YARN_STORE" and first_shop != "YARN_STORE":
    enable_yarn_second_continuation()

if yarn_second and step >= 240:
    use_compatible_yarn_continuation()
else:
    use_v4e_base()
```

## 3. RC3 overlay A — step-241 latent Sheep

At **step 241 (D10H1)** RC3 checks whether the existing programme already contains compatible future `PICKUP SHEEP` / `PLACE SHEEP` capacity.

It then accounts for the market actions already planned in the same turn:

- HIRE cost,
- animals,
- seeds,
- land,
- a cash safety reserve.

Only if there is a real spare physical slot and sufficient remaining liquidity does RC3 increase the existing Sheep purchase by **one**.

This is intentionally much narrower than an earlier generic “buy an extra Sheep” allocator. The earlier version could buy an animal too late in the horizon; RC3 restricts the decision to one early, repeatedly validated compatible slot.

```text
existing choreography has spare Sheep capacity
        +
YARN_STORE is visible
        +
structural spending still leaves > $600 headroom
        ↓
BUY_ANIMAL SHEEP q  →  q + 1
```

## 4. RC3 overlay B — step-385 Fertilizer marginal-value throttle

At **step 385 (D16H1)**, when:

- mode is `BASE`,
- the shed already contains at least **12 Fertilizer**,
- the opponent is still economically active,
- the base action tries to buy Fertilizer,

RC3 reduces the first Fertilizer buy by exactly **one unit**.

It does **not** cancel the whole purchase. More aggressive no-buy versions were rejected because they caused regressions against other strong opponents.

The economic interpretation is marginal value:

```text
large Fertilizer reserve
        ↓
marginal value of one more Fertilizer is low
        ↓
release a small amount of working capital
```

## 5. RC4 overlay — SmartFarm late-liquidity order

RC4 keeps RC3 unchanged for ordinary opponents.

At **step 2**, it can lock a SmartFarm-like family using only information already present in the public observation:

```text
opponent money == 66
opponent farmer == [4, 3]
opponent hands == 5
opponent unlocked quadrants == ["NW"]
```

No player name, replay ID, hidden state, future shop, or seed is used.

If this public-state family is locked, then from **step 577 onward** RC4 only changes the **order** of same-turn market operations:

```text
non-HIRE market operations first
HIRE operations last
```

Quantities are unchanged. Farmer and hand actions are unchanged. The relative order among all non-HIRE orders is unchanged.

This fixes an intra-turn liquidity problem: executing a HIRE too early can consume cash needed for another same-turn market transaction.

## 6. Formal local evaluation

The reviewed RC4 was evaluated in the project's calibrated Kaggriculture **1.32.7 exact-rule runner**, with fresh deterministic seeds and both seat assignments.

| Opponent | W / T / L | Win rate | Mean margin | Worst |
|---|---:|---:|---:|---:|
| SmartFarm | 40 / 0 / 0 | 100% | +3,241.05 | +48 |
| V29-R1 | 40 / 0 / 0 | 100% | +11,767.23 | +1,796 |
| Three-Day Router | 38 / 0 / 2 | 95% | +7,666.50 | -4,787 |
| Where the Wheat Remembers | 36 / 0 / 4 | 90% | +4,964.43 | -3,101 |
| Dusta Champion v2 | 40 / 0 / 0 | 100% | +2,092.15 | +137 |
| v3m | 38 / 2 / 0 | 95% raw wins | +876.62 | 0 |
| opening_q3 | 40 / 0 / 0 | 100% | +2,104.78 | +14 |
| Replay-Distilled v1 | 40 / 0 / 0 | 100% | +2,240.22 | +435 |
| OceanMix Resilient | 40 / 0 / 0 | 100% | +1,981.20 | +112 |
| Shop Guard v46 | 40 / 0 / 0 | 100% | +84,352.18 | +2,305 |

**Aggregate: 392W / 2T / 6L over 400 games = 98.0% raw win rate.**

These are local simulator results, not a claim about a fixed future leaderboard score.

## 7. Build the Kaggle submission

The next cell writes the **exact reviewed source** as `main.py` and creates a deterministic `submission.tar.gz`.

Archive requirements checked here:

1. archive root contains exactly `main.py`;
2. archived `main.py` is byte-identical to the reviewed source;
3. source SHA256 matches the frozen reviewed artifact;
4. archive SHA256 matches the frozen reviewed package.

On Kaggle, run all cells. The file to submit is:

```text
/kaggle/working/submission.tar.gz
```

Outside Kaggle, the notebook writes to the current working directory.

## 8. Submission notes

- **Do not edit the payload cell** if you want the reviewed RC4 artifact.
- The explanatory Markdown does not affect `main.py`.
- Re-running the notebook produces the same deterministic `submission.tar.gz` hash.
- The notebook intentionally does not bundle any external models, datasets, shared libraries, or auxiliary files.
- The final competition artifact is **`submission.tar.gz`**, not the `.ipynb` file itself.

### Final reviewed hashes

```text
main.py
26ffba5273e4432dbc4ec822d5a65b09d3e9f1473c3ce813c528812eeb45779d

submission.tar.gz
8a6e08f0b063f4edf0d72bf6787014eade57f868709b23249ddc9a005696d659
```