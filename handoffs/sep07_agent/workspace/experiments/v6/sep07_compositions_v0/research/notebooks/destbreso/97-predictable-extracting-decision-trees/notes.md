In [a previous notebook](https://www.kaggle.com/code/destbreso/which-language-does-your-agent-speak) I showed how to classify any agent in this competition from a handful of its public replays, by where its cross-game language melts: **SCRIPT** (a frozen plan, recited every game), **BRANCHER** (a repeated trunk that forks on something observed), **SCHEDULER** (a plan composed fresh per game), and a **MIXED** bucket between them, with a complexity index ordering the whole field from reciting to composing.

Of those classes, the BRANCHER is the least complex machine that actually decides anything: a script would need no observations at all, and a scheduler is a whole planner, but a BRANCHER is a plan with a few readable choice points. Which invites going one step further, and that is the question of this page: **can a BRANCHER be reverse engineered from its replays alone?** Not classified, read: at which turns does its plan fork, what observable does each fork condition on, and does the deduced decision tree predict games the agent has never played?

The answer, measured on five agents, two where I know the ground truth and three read blind, is yes: **123 of 126 held-out branch predictions correct, 97.6 percent**, with the known trees recovered fork for fork and variable for variable. That is the 97 percent of the title, and its exact scope so the number cannot oversell: correct held-out predictions of WHICH BRANCH the agent takes, at the forks with enough episodes on each side to test, every such fork of the five-agent study counted and none cherry-picked; it is not a claim about every turn of play, section 7's later case keeps its own ledger, and section 9 lists what stays out of reach.

The plan: first the fair question, whether BRANCHERs even matter at the top (section 1), then the method in three steps (section 2) and its complete code (section 3), a live read of my own agent whose true tree I know by construction (section 4), a historical agent I had reverse engineered by hand months ago, recovered by machine at exact precision (section 5), three top-15 agents read blind with leave-one-out as the judge (section 6), a public ground truth that walked in while this page was being written, an author who published his decision table outright, and what the instrument found when it took that exam (section 7), what the trees have in common (section 8), and the limits (section 9).

The rules of the series hold: every number is computed by the page or carried with its date, the method is complete enough to rerun on any submission id, and what the instrument cannot decide is said out loud rather than smoothed over.

## 1. Are there BRANCHERs at the top?

Before reading anyone's tree, the fair question is whether the class matters at all: perhaps BRANCHERs are a mid-table species and the top belongs to the composers. The census from the previous notebook, the top fifteen teams fingerprinted from their public replays with ranks read on 2026-09-04, says otherwise, and the chart below redraws it for this page's question:

* The one pure BRANCHER in that top 15 was **the rank 1 team of the pull**. The least complex machine that decides anything was, that day, beating everyone.
* Six more of the fifteen are MIXED, and the three this page reads blind in section 6, ranks 2, 5 and 9 at the pull, all carry an early trunk, which is exactly the structure this method needs.
* The historical ground truth of section 5, カワシギ, was one of the strong agents of its era and is a pure BRANCHER on cash.

So the class has not merely appeared at the top, it has held rank 1. A small tree over the right observables is evidently enough to compete with full per-game planners, and that is what makes the question of this page worth asking: a machine that simple, doing that well, should be readable.

## 2. The method: where, what, and does it predict

A BRANCHER repeats a trunk and then forks on something it observes. Reading its tree takes three steps, each with its own failure mode and its own control:

**Where are the forks.** Play the agent's episodes forward together and watch the number of distinct action-prefix classes: every turn where one group of episodes stops agreeing and splits is a fork. This is pure action analysis, no engine involved, and it is the same machinery my language notebook validated blind by recovering seven hand-measured branch turns of a known agent at exact precision with zero false alarms.

**What does each fork read.** At a fork the episodes split into groups; the question is which observable separates the groups. I reconstruct the exact observation the agent saw at the fork by replaying both recorded action streams through the official engine from the recorded seed (a step that must reproduce the recorded final banks to the dollar, or the episode is dropped), then run a decision-stump search over a menu of features: own money, rival money, every product price, own shed, unlocked shops, each at the fork turn, one turn earlier, and one day earlier. A stump that separates the groups perfectly is a candidate rule.

**Does the tree predict.** With few episodes, some feature will separate any split by luck. Two controls: a **permutation p-value** (shuffle the branch labels, count how often some feature separates that well by chance), and **leave-one-out prediction**: refit the stump without one episode, then predict which branch that episode took from its own observation. The LOO count is the number that decides whether a tree is real.

And one honesty rule the first ground truth will demonstrate: **not every fork is a decision.** A weed spawning on a tile reschedules a farmhand and produces a fork that no observable explains; the permutation test is what keeps such noise out of the tree.

## 3. The instrument, complete

Three cells: the fork detector with the stump search and its two controls (pure standard library), the replay puller, and the observation reconstruction through the official engine with the bank-reproduction gate. Point `DEMO_SUBMISSION` further down at any submission id, your own included.

## 4. Ground truth 1: my own agent, read live

The best first test of a reverse-engineering instrument is an agent whose insides I know by construction. My current submission plays a fixed farm plan on a public chassis that routes its MARKET orders on the shops the town unlocks (credited and documented layer by layer in my finance notebook), plus a thin cash-conditioned finance layer of my own. So the true tree is: one big routing fork when the day-6 shop is known, market channel only, and small cash-conditioned sell edits that fire rarely.

The cell below runs the whole pipeline on that submission, pinned to the fourteen episodes of my reference read of 2026-09-05 so the page reproduces its own numbers (use `read_tree(...)` with any submission id, yours included, to read an agent's newest games live). The replay gate prints the engine version it runs on and how exactly each episode reproduces: on kaggle-environments 1.32.7 all fourteen replay to the dollar; on a different build they replay within a stated tolerance, the note says so, and the fork turns, being recorded actions, are exact either way. The read was exactly the build sheet:

* **The routing fork, found**: t=150 (day 6, hour 6), market channel, separated 1.00 by a product price that indexes the unlocked shop, permutation p = 0.0005, leave-one-out 13 of 13. The engine unlocks the second shop at day 6 and the agent's sell routes split right there.
* **The finance layer, seen but honestly unresolved**: money-separated micro-forks at t=206 and t=265, exactly where a cash-conditioned layer edits sells, but at 14 episodes the permutation test refuses to call them (p 0.2 to 0.5). A micro-fork needs a bigger sample than a routing fork, and the instrument says so instead of overclaiming.
* **The noise exhibit**: a plan fork at t=21 splitting one episode from the other thirteen, best feature p = 0.082. That episode's deviation is one farmhand collecting fertilizer a turn early because a weed rescheduled its CARE: environmental noise, correctly left out of the tree.

Leave-one-out over all testable forks: **18 of 18**.

An unpinned rerun over a fresher episode mix later that day added one more build-sheet hit: a plan fork at exactly t=360, which is the turn one of my edit layers un-gates, again below significance at four episodes. Different episode mixes surface different micro-forks; the routing fork and the money family are what reproduce every time.

## 5. Ground truth 2: a hand-derived tree, recovered by machine

Months ago I reverse engineered カワシギ's submission 55425101 by hand, probing branch by branch: seven turns where its plan forks, and the variable it reads, its own cash. That work took days. The instrument gets the same tree in about a minute.

On the same eight public episodes: **all seven fork turns recovered exactly** (engine turns 72, 113, 177, 264, 312, 357, 384), the root fork's best separator is **money** (permutation p = 0.028, and it stays the selected variable down the tree), and leave-one-out on the testable forks is **12 of 12**. The chart below is the recovered tree as a timeline.

Two honest notes. First, at eight episodes the deep forks split two episodes into one and one, which no statistic can bless; the tree's depth is limited by the sample, and the instrument reports those forks with p = 1.0 rather than hiding them. Second, these are recordings from an earlier engine version: they replay within a few hundred dollars rather than to the dollar, so the reconstructed money values carry drift of that order; the fork turns, being recorded actions, are exact. The tolerance and the warning are part of the tool.

## 6. Three top-15 agents, read blind

Ground truths calibrate; the real test is agents I know nothing about beyond their public replays, where leave-one-out is the only judge. Three current top-15 teams whose fingerprints showed a trunk, read from their ten to nineteen newest verified episodes:

| agent | episodes | significant forks (p <= 0.05) | LOO |
|---|---|---|---|
| MtN (rank 5 at the 2026-09-04 pull) | 19 | world forks at t=80 and t=144/150 (p <= 0.011), a CASH fork at t=155 (p = 0.037) | **73/74 = 98.6 %** |
| Giulio Ravasio (rank 9) | 12 | a world fork at t=150 (p = 0.011) | 16/18 = 88.9 % |
| Jesse Bullard (rank 2) | 10 | few testable forks: a scripted plan under a market layer | 4/4 |

MtN is the page's best blind read: a full two-level world router with a cash branch on top, every level predicting held-out games at 19 of 19 or 16 of 17. Its tree is, functionally, a build sheet, and the cell below draws it as one: the whole inferred hierarchy, nodes at their fork turns, leaves at the groups of games each path leads to.

## 7. A public ground truth took the exam

While this page was in review, Thomas Tschinkel published [a notebook](https://www.kaggle.com/code/thomastschinkel/kaggriculture-public-state-router-74-5-win-rate) with the rarest thing this instrument could ask for: an agent whose decision table is written in its own docstring. Three declared forks, public state only: t=226 if a YARN_STORE has unlocked, t=360 if the carrot price is at least 42 (only under yarn), t=433 if milk market inventory is at least 10067 (only under main), plus a prefix guard and three measured repairs. Excellent, independent work: the same prefix-safety and measure-everything discipline this series argues for, arrived at on its own. It is the perfect exam, and it tested more than the tree.

**Round one: his ladder games.** His two live submissions fingerprint identically (a BRANCHER, trunk 0.755 then melt): one agent fielded twice. The tree inferred from those games says: a world fork at t=150 (the yarn worlds buy SHEEP where the others buy COW), sub-splits after, leave-one-out 24 of 24. Nothing at 226, 360 or 433. The instrument and the docstring flatly disagree.

**Round two: the disagreement is the finding.** Replaying his published code turn against turn on his own fielded games settles it: first difference at turn 0, 697 of 719 turns differ. **The published notebook is not the fielded agent**, which is entirely legitimate, a sanitized public release, and invisible to ratings, forums and docstrings alike. The inference of round one was CORRECT for the agent actually playing; behavior outranks documentation, and the instrument is how you find out which one you are looking at.

**Round three: the published code takes the exam properly.** Executed against his own recorded rivals and seeds, twenty episodes, the declared tree comes back: **the t=360 carrot fork returns with the right variable and a fitted threshold of 41.5 against his declared at-least-42, the same rule on integers, recovered blind from seven episodes** (LOO 6 of 7); the yarn fork surfaces at exactly t=226, though only as a residual split, because **his repair layer gets there first**: dead_stock sorts its extra sells by price, prices encode the world, so it separates the yarn worlds at t=151 (p = 0.0005, LOO 20 of 20), an earlier and stronger world-conditioner than the router his docstring counts; and t=433 stays out of reach at twenty episodes, the depth bound of section 9 doing exactly what it says. Repairs are decisions too, whether or not the author books them as such.

This case is kept out of the title's number, which stays pinned to the five-agent study above; folded in, the running aggregate is 187 of 195, 95.9 percent. The chart below puts the three trees of this case on one axis: what he declared, what his published code does, and what his fielded agent does.

## 8. What the trees have in common

Put the five trees on one timeline and the field's BRANCHERs share a skeleton:

* **World forks sit at the unlock turns.** Every first-level routing fork of the five-agent study lands at t=72-80 or t=144-155, days 3 and 6, exactly when the town reveals its shops, and the separating feature is a product price, because the shop draw is encoded in prices. Deeper world conditioning follows within a couple of days (my own tree splits again on a price at t=202). Section 7's router aims deliberately later, t=226 and beyond, and its first behavioral world fork still lands at t=151: a price-reading repair drags even a late router to the unlock. Different teams, same reading points.
* **Cash forks sit on top.** カワシギ's whole tree is cash bands; MtN adds a cash fork after its world routing; my own finance layer edits on cash below significance. Money is the second observable this field conditions on, and so far the only other one.
* **The features that appear are the features the code consults.** In the five-agent study no fork surviving the permutation test is best explained by the rival's state or the shed; those surface only on forks the test rejects as noise. Section 7 then supplies the exception that proves the reading: its agent's dead_stock repair literally reads the shed, and a shed-read fork duly appears in its tree (t=521, p = 0.03). Either a BRANCHER reads the world and its wallet, or, when it reads something else, the tree says so.

That last sentence is a finding about the LADDER, not about the game: the game offers a rival to read, and the BRANCHERs of the top mostly do not read it.

## 9. Limits, and what a reader should not conclude

* **Depth is bought with episodes, and the LOO count says how far to trust.** Section 7's t=433 fork stays out of reach at twenty episodes because the earlier splits fragment the sample below the recursion floor; the same current number one that reads as singleton mush at ten episodes reads as a sharp tree at forty in my x-ray notebook's live section. A fork that splits two episodes one against one carries no statistics, and the instrument prints it with p = 1.0 rather than hiding it.
* **Attribution names the mechanism family and the band, not always the exact variable.** World proxies tie with each other (several prices, and the money footprint the world leaves in an opening), and own money is indistinguishable from rival money at small n because the two drift together. The ties are printed, never hidden; at these sample sizes the honest claim is often "a world read at day 6" or "a cash read near t=200", not the variable's name.
* **It reads behavior, not intention.** A repair and a router are both forks to this instrument. In section 7 that was the virtue, it surfaced a price-reading repair the author does not book as a decision; in general it means the extracted object is the BEHAVIORAL tree, which can be larger than the tree the code's author would draw.
* **It applies to BRANCHERs, and says so when it does not.** The method needs a trunk and a floor of episodes (eight at the very least, more for deep trees). On a SCRIPT there is nothing to read, on a SCHEDULER there is no trunk to anchor on, and the conditional cell in my x-ray notebook stands down with the class instead of producing noise.
* **Micro-forks need more games than routing forks.** My own finance layer is real and fires exactly where the instrument sees money splits, and fourteen episodes still cannot certify it. Absence of significance is not absence of the branch.
* **A readable tree is not a weak agent.** Reading a rival's tree tells you what it will do; beating it is a separate problem. This page measures identity, not strength, like the rest of the series.
* Replays, seeds and standings come from the public Kaggle endpoints; observations are reconstructed by replaying recorded actions through the official kaggle-environments engine (1.32.7 at the time of writing) with a bank-reproduction gate per episode. Credit to カワシギ, MtN, Giulio Ravasio, Jesse Bullard and Thomas Tschinkel, whose public games and, in the last case, publicly documented agent are the material, and to the chassis author credited in my finance notebook. Companion pages: [Which language does your agent speak?](https://www.kaggle.com/code/destbreso/which-language-does-your-agent-speak) and [X-ray your agent](https://www.kaggle.com/code/destbreso/x-ray-your-agent).