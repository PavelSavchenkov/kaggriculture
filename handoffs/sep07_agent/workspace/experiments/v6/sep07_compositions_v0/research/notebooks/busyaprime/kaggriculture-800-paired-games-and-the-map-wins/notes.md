# Kaggriculture, 800 paired games and the map wins

Eight hundred games, played locally on the competition engine, that put one question to the strongest
public agent on this tab: can a model of the market choose its tape better than the hand map its author
fitted on real matches? The agent is yhay81's two-shop tape router. It replays one of ten recorded
719-action histories and picks which one at step 144, by the first two shops the town has revealed. We
replaced only that choice, three different ways, and played every variant against the original on
identical seeds, seats and opponents.

The answer is no, and the reasons are worth more than a yes would have been. A market model that
reproduces season revenue to 76 coins when it knows the opponent's schedule, and to 4058 when it uses
the field average, still picks tapes that bank 3662 to 4832 coins less per game, and the shared-market
choice loses 11.0 points of win rate. Across 31 shop pairs, the gain the model expected and the bank
difference the games produced are unrelated: a Spearman correlation printed as -0.000.

What a fork gets: the exact market model, the shop-draw posterior, three runnable variants of the
router, and a paired evaluation harness that turns any change to an agent into a difference with a
confidence interval. All of it credits yhay81, whose router is used under Apache 2.0 with one table
replaced.

## 1. One router, one decision

The router carries ten tapes and two branch points, at steps 72 and 144. The first branch sends seven
of the eight possible first shops to tape 0 and only a first YARN_STORE to tape 1. The second branch is
a 64-entry map from the ordered pair of the first two shops to a tape, and it sends 33 of the 64 pairs
to tape 0. The matrix below says why nothing later than step 144 can be a decision: tape 1 parts from
every other tape at step 73, and every other pair of tapes parts by step 150 at the latest. No two
tapes are still identical at step 216, when the third shop appears. A tape switched after 144 would be
replaying a history whose land, animals and plantings the farm does not have. So every variant in this
notebook changes the 64-entry map and nothing else.

The profile table is what the map chooses between. Tape 8 sells 296 units of wool and no carrots; tape
7 sells 79 units of wool and 423 of wheat; tape 5 sells the most milk and strawberry, 289 and 326 units.
Those differences are the whole lever.

## 2. A market model exact to the coin, alone

The market model reproduces the engine's rules: the price response to inventory, the depth each shop
adds, the sell ticks, the shop unlocks, and the uniform draw of the shops not yet revealed, which enters
through the posterior our shop-draw notebook published. The table scores it on 120 games against real
opponents, one forced tape per game, revenue actually earned against revenue expected. The solo version,
which assumes the farm sells alone, is off by 52026 coins on average and always upward. Give the same
model the opponent's actual executed schedule and the error falls to 76 coins. Replace that schedule
with the pool's average and it is 4058. Integrate over the shop posterior, as the router must at step
144, and it is 14140. The market is shared; that is the whole difference between the two panels.

## 3. What the model recommends

Two tables, one per model, each naming a tape for every one of the 64 ordered shop pairs. The table
built from executed sales disagrees with yhay81 on 48 pairs, 75.0 per cent of the draw mass, and sends
41 of them to tape 9, the wool tape. The table built from the shared-market model disagrees on 45
pairs, 70.3 per cent, and splits between tape 4 in 26 pairs and tape 5 in 21. yhay81's own map sends 33
pairs to tape 0. The product breakdown of the ten largest expected gaps is the tell. Where the model
prefers tape 6 its edge over tape 0 is 12016 coins of milk; where it prefers tape 9 the edge is 6971
coins of wool plus 3701 of fertilizer. Wool is the product our shop-draw notebook found without a buyer
in a third of towns, and that is the product the executed-sales table bets on.

## 4. Eight hundred paired games

Four opponents from the public tab, kaito48, v16rc5, k320 and closer_cleo; 25 seeds each; both seats;
the same seed, seat and opponent for the original and for every variant, so each difference below is a
paired difference. Two hundred games per agent, 800 in all.

Variant B, the executed-sales table, wins 2.5 points more often, an interval from -1.5 to +6.5 that
covers zero, and banks 4066 coins less, an interval from -7420 to -712 that does not. Variant C, the
shared-market table, loses 11.0 points, from -18.3 to -3.7, and 4832 coins; against kaito48 alone it
loses 28.0 points. Variant D, the wool tape in every town, is 1.0 point up and 3662 coins down.
Restricted to the pairs where a variant actually differs from yhay81, every interval widens and every
sign holds. The original wins 94.0 per cent of these games; the pool is not strong enough to separate
B and D from it on wins, and it is strong enough to show that all three earn less.

## 5. Why an exact model still loses

Take variant C, the one built on the model that is exact when it knows the opponent. For each of the
31 shop pairs where C's tape differs from yhay81's and games were played, the table lists the gain the
model expected and the bank difference the games produced. Twenty-three of the 31 pairs went the wrong
way. The Spearman correlation between expected and realised prints as -0.000 with p = 0.998; Pearson
is +0.046 with p = 0.808. The model is not noisy. It is aimed at the wrong quantity.

A game is won on the bank difference against one particular opponent, and that opponent's schedule is
not the field average. It sells into the same shops at the same steps, and which tape survives that
contact depends on the opponent's tape, not on the town alone. An expectation over the field is the
right number for a lone seller and the wrong number for a duel. yhay81's map was fitted on the outcomes
of recorded matches, so it already contains the duel. A revenue model, however exact, does not.

## 6. What this shows and what it does not

It shows, with paired intervals, that three model-driven replacements of the second-branch map earn
less than the map, and that one of them loses games. It shows the loss is not a modelling error, since
the same market model is exact to 76 coins with the opponent's schedule in hand. And it shows that the
model's expected gain per shop pair has no relation to the realised bank difference.

It does not show that the map is optimal. The pool is four public agents, not the live ladder; 25 seeds
per opponent give intervals of about 4 points either way on wins; and the original already wins 94.0
per cent of these games, so a gain on wins would be hard to see here even if it existed. A stronger
pool, and a variant that changes the tapes rather than the routing, are the two experiments this
harness makes cheap. What would overturn the conclusion is a variant that beats the original on wins
with an interval clear of zero, against a pool that includes the current top of the ladder.

## Fork notes

Everything is in the attached dataset. results.csv has one row per game: seed, seat, opponent, agent,
both banks, the win flag and the town's shops. table.json and table2.json hold the expected revenue of
all ten tapes for all 64 pairs under each model, with the tape each model picks and the tape yhay81
picks. validate2.json is the 120-game validation. revenue.py and revenue2.py are the market models,
shop_draw_model.py the posterior, eval_paired2.py the paired runner; the last carries local paths, so
point them at your own copies. main_posterior.py, main_shared.py and main_wool.py are the three
variants, each yhay81's file with one table replaced and a header that says so.

Credits. The router is
[yhay81's Public Match History Router](https://www.kaggle.com/code/yhay81/public-match-history-router-rating-2929-aug-30),
Apache 2.0. The opponents are public agents by kaitofukami, boatlee and raykkretzschmar, whose
reference agents also supplied closer_cleo. The posterior and the market-depth numbers come from two
earlier notebooks of mine on this competition,
[one town in three has no yarn store at all](https://www.kaggle.com/code/busyaprime/one-town-in-three-has-no-yarn-store-at-all)
and
[the best thing to farm has a market of 59 units](https://www.kaggle.com/code/busyaprime/the-best-thing-to-farm-has-a-market-of-59-units).