# Six-Day Public-State Fieldbook

This is a small public-state router around strong public action histories. It
does not try to invent all 720 actions online. Instead, it follows a reviewed
six-day plan, looks at the farm and market, and chooses the next six-day plan.

## What it does

There are 24 turns per day. The season is split into five 144-turn blocks:

| period | turn | available plans | decision |
|---|---:|---:|---|
| day 0–6 | 0–143 | 1 | fixed opening |
| day 6–12 | 144–287 | 2 | public-state tree |
| day 12–18 | 288–431 | 1 | fixed bridge |
| day 18–24 | 432–575 | 3 | public-state tree |
| day 24–30 | 576–719 | 5 | public-state tree |

At a boundary, the router uses only information already available in the
observation: unlocked shops, market prices and inventory, public farm state,
and its own private shed and hand inventories. It never keys on a player name,
leaderboard rank, replay id, random seed, final result, or a future shop.

## How I built it

1. I downloaded the current top 200 and older public replays, then deduplicated
   complete action histories by exact SHA-256.
2. I collected every available public episode for rank 3 Bantam (83) and rank
   8 Zenith Ye (119), and treated their six-day blocks as swap candidates.
3. I tested candidate blocks at the same boundary, on fresh seeds and both
   seats, rather than comparing unrelated full games.
4. I retained eight distinct routes and fitted small decision trees using only
   public runtime features.
5. I froze the policy and evaluated it against 188 external histories from the
   current top-200 snapshot.

The search also used 400 historical top-200 episodes (363 unique complete
histories). Historical results were used to propose routes; fresh seed blocks
were kept for the final measurements.

## Four decisions, eight routes, thirty possible paths

The agent can switch at turns 144, 288, 432, and 576. Across the four deployed
trees there are 16 binary tests and 20 leaves. The route counts are
`1 × 2 × 1 × 3 × 5`, so there are 30 possible full-season route sequences.
Only eight route tapes are stored because the same base route is reused across
several blocks.

Typical tests ask questions such as “is carrot expensive?”, “is fertilizer
cheap?”, or “is the cash gap positive?”. They are ordinary comparisons against
the current observation, not opponent identification.

## Budget guard

Before starting a new block, the agent calculates the cost of purchases already
written into that block: hires, land, seeds, feed, fertilizer, and animals. If
cash is sufficient, it changes nothing. If cash is short, it protects the
block's required stock and sells only excess products, highest current price
first. This avoids blindly liquidating useful feed or fertilizer just because a
boundary was reached.

## Evidence and limits

On the frozen release block—188 opponents, 64 fresh seeds, both seats—the agent
recorded 22,795 wins, 58 draws, and 1,211 losses in 24,064 games: a 0.9485 point
rate with seed-bootstrap 95% CI `[0.9371, 0.9565]`. Across two fresh blocks it
recorded a 0.9503 point rate over 36,096 games. There were no runtime or schema
errors.

This is local evidence, not a leaderboard guarantee. The regular 164-opponent
subset scored 0.9948, while the deliberately hard 24-opponent subset scored
0.6322. The hard tail remains the main place to improve.

## Readable notebook, separated tapes

The long C++ action tables are intentionally not printed in this Notebook.
They live in the attached public source Dataset with hashes and an Apache-2.0
license. The cells below verify every source file, compile the Linux agent, and
leave `main.py`, `agent.so`, and a top-level two-file submission archive in
`/kaggle/working`.


## Build the reviewed agent

The long action tapes and C++ implementation live in the attached public **Six-Day Public-State Agent Source** Dataset. Keeping them there makes this Notebook short enough to read while preserving a hash-verifiable build.
