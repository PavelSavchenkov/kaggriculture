# Kaggriculture: Public State Router (74.5% Win Rate)
## Breaking the Replay Ceiling with Safe Branching and Online Repairs

Most top bots right now replay fixed games because manual online planning is too slow. But fixed routes fail when market prices or town shops change, because their was no way to know the shop unlocks in advance. Splicing random games also fails because the whole farm layout depend on the early turns.

My approach for v3.1:
1. Prefix Guarded Tail Switching: I only switch between routes that share the exact same opening turns. Zero tile desync.
2. Three Public State Triggers: I branch based only on unlocked town shops and market prices.
3. Three Online Repairs: Quick fixes for weeds, dead stock, and unfillable sells (+19 wins and 0 losses on my 968 game benchmark).

Results on 968 real ladder games:
1. Best Single Route: 62.0% HOLD
2. With Public State Router: 70.6% HOLD
3. With Online Repairs (Shipped v3.1): 74.5% HOLD (+12.5% boost)

Against top 5 ladder teams: 66.3% win rate taking their opponents seat.


## 1. How the Decisions and Repairs Work

The agent starts on a strong baseline route. At three turns it checks the public observation to pick a tail:
1. Turn 226 (day 9.4): YARN STORE unlocked in town, switch to wool tail.
2. Turn 360 (day 15.0): Under yarn, carrot price at least 42, switch to carrot tail.
3. Turn 433 (day 18.0): Under main, milk market inventory at least 10067, switch to milk tail.

A runtime prefix guard blocks any switch if the opening turns do not match 100%.

Three Online Repairs:
1. weed dig: An idle unit standing on a weed digs instead of doing nothing.
2. dead stock: Sells leftover produce before turn 720 so money does not rot in the shed.
3. clamp sells: Drops sell orders that the shed cannot fill so you dont loose market slots.


## 2. Benchmark Numbers and What Failed

Results across 968 ladder games:
1. Best Single Route: 65.1% SEARCH, 63.6% SCREEN, 62.0% HOLD
2. With Router: 75.4% SEARCH, 73.5% SCREEN, 70.6% HOLD
3. With Router plus Repairs: 77.0% SEARCH, 74.3% SCREEN, 74.5% HOLD

Against Live Reacting Bots (120 games each on fresh seeds):
1. Against a top reacting bot: 57.5% jumped to 70.0%
2. Against another top ladder bot: 73.3% jumped to 80.0%
3. Against a third public bot: 100.0% remained 100.0%
4. My older submission v2.8: 43.3% jumped to 56.7%

Ideas I tested and rejected (hurt the HOLD win rate):
1. Sells before buys: Lost 17 games on HOLD.
2. Hire last: Lost 13 games on HOLD.
3. Deferring unaffordable buys: Neutral or negative.
4. Feeding and caring on idle turns: Lost 4 games on HOLD because shed capacity overflowed.


## 3. Standalone Agent Code (main.py)

Writes the complete standalone agent into main.py using standard python libraries.


## 4. Local Test Run (720 Turns)

Running a quick 720 turn test match with kaggle environments.


## 5. Build submission.tar.gz

Create the tar archive containing main.py for submission.


## Summary

1. Offline routes solve the hard logistics that online planners struggle with.
2. Prefix alignment is the only safe way to switch routes without desync.
3. Targeted repairs give big gains (+19 wins and 0 losses on my test panel).

If this notebook helped your ladder climb, please leave an upvote. Good luck!
