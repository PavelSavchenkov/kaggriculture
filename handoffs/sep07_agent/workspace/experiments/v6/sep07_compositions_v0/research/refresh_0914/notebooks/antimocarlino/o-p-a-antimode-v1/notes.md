```
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║     ▓█▀▀▀▓  ▓█▀▀▀▓  ▓█▀▀▀▓         ▄▀█ █▄░█ ▀█▀               ║
║     ▓█  ▓█  ▓█▀▀▀▓  ▓█▀▀▀▓         █▀█ █░▀█ ░█░               ║
║     ▓█▄▄▄█  ▓█      ▓█▄▄▄█         ▀░▀ ▀░░▀ ░▀░               ║
║                                                                ║
║        O B S E R V E  ·  P L A N  ·  A D V A N C E             ║
║                                                                ║
║     ═══════════════════════════════════════════════════        ║
║     S I L E N T   S T R I K E   P R O T O C O L               ║
║     ═══════════════════════════════════════════════════        ║
║                                                                ║
║     "I observe when others move.                               ║
║      I plan when others rush.                                  ║
║      I advance when others stop."                              ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝
```

<div align="center">

# 🎖️ O.P.A. — ANTIMODE

### *Silent Strike Protocol — An ANTIMODE Framework Agent for Kaggriculture*

</div>

# 🎖️ O.P.A. — ANTIMODE

### Silent Strike Protocol - An ANTIMODE Framework Agent for the Kaggriculture Competition*

> **"I observe when others move. I plan when others rush. I advance when others stop.  
> Calm is my discipline. Method is my weapon. Vision is my advantage."**

---

## 📡 Executive Summary

**Operation O.P.A.** (Observe · Plan · Advance) is an autonomous AI agent built for the **Kaggriculture** competition. It does not chase the market — it waits for the market to reveal itself. Inspired by the **ANTIMODE framework**, the agent operates as a *silent strategist*: it accumulates resources in silence, identifies strategic windows with surgical precision, and strikes aggressively only when the advantage is mathematically certain.

**Core Philosophy:** Hybrid behavior — *observe like a conservative, strike like an aggressor.*

---

## 🧭 The ANTIMODE Philosophy

ANTIMODE is not a machine learning model. It is an **operational mindset** designed for complex, dynamic, non-linear environments where:

- The context is alive and reactive
- Resources are scarce and must be conserved
- Timing matters more than speed
- Predictability is a weakness; adaptability is strength

The Kaggriculture competition is the **perfect theater** for this doctrine: planting crops, managing resources, raising animals, and trading in a market that reacts to every move. It is not a race of brute force — it is a war of patience, pattern recognition, and surgical execution.

---

## 🎯 The Three Mental States

The agent's mind is a **state machine** with three operational modes. Transitions are driven by market signals, resource levels, and volatility — never by impulse.

### 🟦 CALMA — *Observe*
> *"I observe when others move."*

- **Purpose:** Gather intelligence. Zero waste. No impulsive moves.
- **Behavior:**
  - Buy only essential seeds
  - Avoid land expansion
  - Monitor prices, demand, and market patterns
  - Build a cash buffer above `SAFE_CASH`
- **Exit condition:** Market stabilizes + cash reserve secured → transition to `FOCUS`

### 🟨 FOCUS — *Plan*
> *"I plan when others rush."*

- **Purpose:** Prepare the strike. Methodical selection of high-ROI actions.
- **Behavior:**
  - Identify crops with best risk/reward ratio
  - Plan irrigation and harvest sequences
  - Evaluate land expansion opportunities
  - Prepare batch sales for the next window
- **Exit condition:** Price window opens above target → transition to `ADVANCE`

### 🟥 ADVANCE — *Strike*
> *"I advance when others stop."*

- **Purpose:** Exploit the window. Surgical aggression.
- **Behavior:**
  - Sell all ready stock at peak prices
  - Expand land if ROI exceeds threshold
  - Invest in high-margin animals
  - Exploit favorable volatility
- **Exit condition:** Volatility spikes OR cash drops below safe level OR strike window expires → return to `CALMA`

---

## ⚙️ Operational Thresholds

| Parameter | Value | Purpose |
|---|---|---|
| `SAFE_CASH` | 250 | Minimum cash reserve — the agent never fights without a buffer |
| `PRICE_TARGET_MULTIPLIER` | 1.15 | Strike only when price ≥ 115% of 10-turn average |
| `VOLATILITY_THRESHOLD` | 0.20 | Retreat to CALMA if price swings > 20% |
| `MARKET_STABILITY_WINDOW` | 6 turns | Confirm stability before entering FOCUS |
| `LAND_BUY_CASH_BUFFER` | 600 | Extra margin before spending 1000 on land |
| `HIRE_CASH_BUFFER` | 300 | Extra margin before hiring a hand |
| `ANIMAL_UNLOCK_CASH` | 1500 | Animals are the *last* investment — only after cashflow is solid |

---

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                  OBSERVATION LAYER                  │
│  • Market prices  • Tile states  • Resource levels  │
│  • Competitor moves  • Volatility signals           │
└──────────────────────┬──────────────────────────────┘
                       ▼
┌─────────────────────────────────────────────────────┐
│            ANTIMODE STATE MACHINE                   │
│         CALMA  ──►  FOCUS  ──►  ADVANCE             │
│           ▲                              │           │
│           └──────────────────────────────┘           │
└──────────────────────┬──────────────────────────────┘
                       ▼
┌─────────────────────────────────────────────────────┐
│              EXECUTION LAYER                        │
│  • Farmer movement  • Hand coordination             │
│  • Tile actions (plant/water/harvest)               │
│  • Animal management  • Market orders               │
└─────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Cashflow First, Expansion Later** — The agent builds a solid financial cushion before unlocking Melon, Strawberry, or animals.
2. **Per-Player State Isolation** — Each player instance maintains its own state and price history, preventing cross-contamination in mirror matches.
3. **Collision Avoidance** — Hands avoid the farmer's target tile, reducing wasted movements.
4. **Single Strategic Spend Per Turn** — Land → Hire → Animal, in strict priority order. No overlapping big expenses.

---

## 📊 Performance Results

All tests run with fixed seeds for fair comparison. Episode length: 720 steps.

### Head-to-Head Results

| Opponent | O.P.A. Score | Opponent Score | Outcome |
|---|---|---|---|
| `random` | 9,899 | 0 | ✅ **VICTORY** |
| `starter` | 10,651 | 2,524 | ✅ **VICTORY** |

### Multi-Seed Robustness (vs `starter`)

| Seed | O.P.A. Score | Opponent Score | Margin |
|---|---|---|---|
| 1 | 11,073 | 2,493 | +8,580 |
| 2 | 10,575 | 2,487 | +8,088 |
| 3 | 9,449 | 2,516 | +6,933 |
| 7 | 10,651 | 2,524 | +8,127 |
| 13 | 7,133 | 2,518 | +4,615 |

**Average margin of victory: +7,268 points**

### Day-by-Day Evolution (vs `starter`, seed 7)

| Day | O.P.A. | Starter | Delta |
|---|---|---|---|
| 0 | 2,000 | 2,000 | 0 |
| 12 | 619 | 2,138 | −1,519 |
| 18 | 3,930 | 2,264 | **+1,666** ← First overtake |
| 22 | 5,341 | 2,308 | +3,033 |
| 26 | 6,391 | 2,458 | +3,933 |
| 28 | 10,215 | 2,438 | **+7,777** |
| Final | 10,651 | 2,524 | **+8,127** |

> The agent deliberately falls behind in the early game (CALMA phase), then overtakes decisively once the FOCUS → ADVANCE transition triggers. This is the ANTIMODE signature: *lose the first battle, win the war.*

---

## 🚀 How to Run

### Local Testing

```python
from kaggle_environments import make

env = make("kaggriculture", configuration={"episodeSteps": 720, "seed": 7})
env.run([agent, "starter"])
print([s.reward for s in env.steps[-1]])
```

### Kaggle Submission

1. Upload the `submission.py` file containing the `agent(obs)` function.
2. Select the Kaggriculture competition.
3. Submit and monitor the leaderboard.

---

## 🧠 The Mantra (Operational Doctrine)

> *"I observe when others move.*  
> *I plan when others rush.*  
> *I advance when others stop.*  
>   
> *Calm is my discipline.*  
> *Method is my weapon.*  
> *Vision is my advantage."*

This is not just a strategy — it is the **internal logic** of the algorithm. Every line of code is a translation of this doctrine into executable behavior.

---

## 📜 License

This project is released under the MIT License. Feel free to study, adapt, and deploy — but remember: **the silent strategist always wins in the end.**

---

## 🙏 Acknowledgments

- **Kaggle** for hosting the Kaggriculture competition.
- **The ANTIMODE framework** — the philosophical backbone of this agent.
- Every opponent who rushed, panicked, and overextended — you taught the agent when to strike.

---

<div align="center">

### 🎖️ *Operation O.P.A. — Silent Strike Protocol*  
*Observe. Plan. Advance.*

</div>

## 🎖️ After-Action Report — The Anatomy of a Silent Strike

*Operational analysis of the match vs `starter` (seed 7), where O.P.A. converted a -1,519 deficit into a +8,127 victory.*

---

### 📅 Phase I — The Silent Accumulation (Day 0-16)

**Mental State:** 🟦 `CALMA` → 🟨 `FOCUS`

| Metric | Value |
|---|---|
| Cash at Day 12 | 619 (vs 2,138 opponent) |
| Max deficit | -1,680 (Day 4) |
| Strategic behavior | Zero expansion, seed-only spending, price monitoring |

**What the agent was doing:**
- Planting CARROT cycles for steady micro-cashflow
- Tracking WHEAT prices to detect stability windows
- Refusing every impulse to expand or hire
- Building the `price_history` buffer required by `market_is_stable()`

> *"I observe when others move."* — While `starter` spent aggressively, O.P.A. was **loading the weapon**.

---

### ⚡ Phase II — The Strike Window (Day 18)

**Mental State:** 🟨 `FOCUS` → 🟥 `ADVANCE`

| Metric | Day 16 | Day 18 | Delta |
|---|---|---|---|
| O.P.A. cash | 1,488 | 3,930 | **+2,442 in 2 days** |
| Starter cash | 2,180 | 2,264 | +84 |
| Gap | -692 | **+1,666** | **First overtake** |

**The trigger sequence:**
1. `market_is_stable()` confirmed WHEAT stability over 6 turns
2. Cash exceeded `SAFE_CASH` (250) → transition to `FOCUS`
3. `good_window()` detected price ≥ 115% of 10-turn average → `ADVANCE`
4. Batch sell of accumulated stock at premium prices
5. `MELON_UNLOCK_CASH` (700) reached → 5 tiles converted to high-margin MELON
6. `BUY_LAND` authorized (cash - 600 buffer ≥ 1000)

> This single day is the entire ANTIMODE doctrine compressed into 48 turns: **16 days of silence, 2 days of surgical violence.**

---

### 🚀 Phase III — Controlled Domination (Day 18-28)

**Mental State:** 🟥 `ADVANCE` (with automatic `CALMA` retreats on volatility spikes)

| Day | O.P.A. | Starter | Margin |
|---|---|---|---|
| 20 | 4,354 | 2,328 | +2,026 |
| 22 | 5,341 | 2,308 | +3,033 |
| 24 | 5,713 | 2,392 | +3,321 |
| 26 | 6,391 | 2,458 | +3,933 |
| 28 | 10,215 | 2,438 | **+7,777** |

**Compounding effects:**
- MELON tiles producing 6 units × high market price
- Second land quadrant unlocked → doubled working surface
- Hired hands multiplying tile actions per turn
- `ANIMAL_UNLOCK_CASH` (1500) reached → COW purchased for milk revenue

> *"I advance when others stop."* — `starter` plateaued at ~2,400. O.P.A. was just getting started.

---

### 🎯 Key Takeaway

The victory was not decided on Day 28. It was decided on **Day 4**, when the agent was 1,680 behind and **chose not to panic**.

That is the ANTIMODE signature:

> *Lose the first battle. Win the war.*