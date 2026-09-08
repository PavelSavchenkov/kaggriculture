# Replaying someone else's tape gets you 88% of their score

Almost everyone in this competition is running a fixed program. I measured it: on 26 real replays of my scoring submission, 25 of the 26 opponents were playing a tape.

So the obvious move is to find the strongest tape and play that one. I spent a while on it. Here is the number that ended the search, and I think it saves you the same detour.

**Replaying a tape reproduces about 88% of its owner's score.** Not 100. And that 12% gap is enough to put gold out of reach for every tape in the competition, including the one belonging to the team in first place.


## The measurement

Two teams, replayed in full, scored against the same opponents.

| tape replayed | fraction of the owner's score I recovered |
|---|---|
| tiki | **92.0%** |
| gogogo | **84.8%** |
| average of the two | **88.4%** |

My first guess was that the loss came from the reactive part. A tape is a fixed sequence, so anything the owner's agent did in response to what opponents were doing is gone when I replay it.

That guess was wrong, and the check is simple. I replayed gogogo's tape against gogogo, in both seats. Self-similarity 100.0%, both seats, and it still lost 15%.

So the missing 12% is not reactivity. Something else about being the tape's owner is worth 12%, and I could not recover it by copying harder.


## What that implies about the ceiling

Work it backwards.

Gold sits at 2686.9 (rank 24 of 7088). To reach that by replay, at 88% recovery, you would need a tape whose owner scored:

```
2686.9 / 0.88 = 3053
```

The highest-scoring team in the whole competition, tetsuya, is at **2924.5**.

There is no such tape. Copy the single best player in the competition and you land at roughly 2585, which is **102 points short of gold**.

The entire family of copy-the-tape strategies is capped below the medal line. That is not a statement about which tape to pick. There isn't one.


## Where I actually am, and why I stopped looking

I finished at **2398.1, rank 175 of 7088**. Comfortable silver, 289 points off gold.

For a while I kept probing tapes, on the theory that a better one existed. The arithmetic above is what stopped me. The best possible outcome of that search was 2585, and I was already close enough to it that the remaining upside was smaller than it looked.

That reframes the problem. Gold does not come from a better tape. It comes from an agent that generates both halves of the game itself, the farming and the market, and reacts.


## Two things worth knowing if you go that way

**96% of your opponents are tapes.** That is not a complaint, it is an exploitable fact. You are playing against fixed sequences that will not adapt to anything you do. An agent that notices what the field is doing has 25 of 26 opponents who cannot notice back.

**Score in coins, not in wins.** I nearly fooled myself here. A change can win more games and still lose money, because the losses get worse. Measure the thing the leaderboard measures.

And if you are benchmarking anything, pin the opponents. The engine draws shop stock from the same random stream as the weeds, so two runs with the same seed can still land in different towns and swing the result by twenty thousand coins. I lost a day to that before I pinned them.


## Work out your own ceiling

Plug in what you measure and it tells you whether copying can reach the score you want, and what recovery rate you would need if it can't.

The only input you have to measure yourself is the recovery rate: replay someone's tape, score it, divide by their score. Two replays is enough to get a working figure.


## The short version

If you are copying tapes: the ceiling is 88% of the best tape available, which is 2585, which is 102 short of gold. Stop tuning which tape.

If you have a replay that recovers more than 92%, I would like to see it. That would break the arithmetic above, and it is the only thing that would.
