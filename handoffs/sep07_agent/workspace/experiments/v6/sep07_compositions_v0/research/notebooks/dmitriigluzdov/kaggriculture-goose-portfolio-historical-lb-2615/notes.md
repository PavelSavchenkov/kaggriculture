# Kaggriculture Goose Portfolio | Historical LB 2615

**A complete agent, with a small economic decision worth reusing.**

Should a farm buy two cows, one cow and one goose, or two geese? Extra eggs earn money,
but they also lower the price received by the rest of the herd. This agent compares the
three choices before changing its purchase and placement plan.

**Run all cells to create `main.py` and `submission.tar.gz`.** The archive is ready to
select for a Kaggriculture submission. Running this notebook does not submit it for you.
The default settings recreate our S38 file byte for byte.

**About 2615.0:** this is the historical public LB score of S38, submitted on 29 August
2026 (ref `55859195`), as observed on 6 September. It is not a guaranteed score for a new
submission. The same code later received 1833.3 under a different submission.

The base farm controller comes from [The Moon Counts Melons — prvsiyan, V29](https://www.kaggle.com/code/prvsiyan/kaggriculture-frontier-the-moon-counts-melons?scriptVersionId=345469411),
under Apache-2.0. Our additions handle livestock valuation, compatible purchases and
placement, market ordering, and the second-goose guard explained below.

## 1. Quick start

1. Leave the settings below unchanged for the historical agent, or edit them for an experiment.
2. Run all cells. The notebook checks the source, runs complete local games, and builds the archive.
3. Save a completed version. On the competition's **Submit Agent** page, select that
   notebook version and `submission.tar.gz`, or download and upload the archive yourself.

No GPU or external dataset is needed. Enable Internet for the notebook: the setup cell
installs `kaggle-environments==1.32.7` if needed. Kaggle's default image can contain an
older game version with different mechanics. The submitted agent itself uses only the
standard library and needs no internet.


## 2. Why the second goose needs a buffer

The model estimates the remaining livestock revenue for all three portfolios, including
the existing herd. It subtracts the new animals' purchase cost and an allowance for feed
and worker time. This catches a common mistake: valuing extra production without counting
the lower price on production the farm already has.

| Choice | Historical rule |
|---|---|
| Consider geese | An egg-consuming shop is already visible |
| Move away from two cows | The alternative must beat them by at least 600 projected coins |
| Keep two geese | The second goose must add at least 2,000 over the mixed herd |

If two geese fail the second test, keep one goose only if the mixed herd still clears
the first test. Otherwise keep two cows. These buffers are heuristics, not universal
constants. The examples below use invented values so the boundary is easy to inspect.


### Where the decision changes

The axes are forecast advantages over two cows, in coins—not leaderboard gains.
This map assumes visible egg demand and space for two animals.


### Try a different market

The next two cells expose the historical valuation functions and an invented farm.
Changing the starting egg stock, with everything else fixed, moves the recommendation
from two geese to a mixed herd and then to two cows. These stock values were chosen to
show the transitions; they are not a sample of competition games.

The estimate assumes daily feeding and care, approximate rival production, and a
30-day season. Unknown future shops receive a discounted demand estimate (weight 0.35).
It does not simulate worker travel, failed purchases or shed overflow. Treat its output
as a comparison between choices, not an exact profit forecast.


## 3. Buying the animal is only the first step

The full agent makes this decision at step 150, where its existing schedule can support
two new animals. A first-shop Yarn Store follows the inherited sheep plan instead.

| Stage | What the controller must preserve |
|---|---|
| Buy | Enough cash when the order executes, within the ten-order limit |
| Build | A coop for each goose; a pasture for each cow |
| Pick up and place | The correct animal, worker and destination |
| Feed and harvest | Enough wheat and worker turns for the expected production |
| Sell | Actual shed stock, without displacing a necessary market order |

A mixed purchase needs an extra market order. In this particular schedule, one strawberry
seed order can move from step 150 to 151, before planting at 155. That timing is part of
the controller, not a general trick to copy into another route.

The implementation tracks intended replacements and checks observed inventory later.
It does not guarantee that every route succeeds against every opponent. End-of-game
selling covers reachable shed stock, not all carried goods or placed animals.

## 4. Full agent source

The following cells store the complete source and then write `main.py`. The inherited
Moon controller is kept together; our readable additions follow it. Keeping the original
bytes lets the default build check itself against the file that earned the historical score.

For a deeper change, edit the source strings below. `_e157_projection` estimates value;
`_e157_allocate` handles allocation and placement; the final portfolio wrapper limits
the number of geese. Editing the teaching examples alone does not change the agent.
The first two settings above do change the packaged file.

The inherited controller includes a fixed action schedule and repairs based on observations.
It is not a learned policy or a planner that rebuilds the entire farm route each turn.


### Inherited Moon controller


### Market and inventory support


### Livestock valuation and compatible allocation


### Joint portfolio guard


## 5. Check the complete agent

These four games run the generated file against the built-in starter, using two seeds
and both player positions. Each game loads a fresh module so state cannot leak between
runs. The check fails on exceptions, malformed actions or incomplete games.

This is a compatibility check, not a competitive benchmark. Before relying on a change,
use more seeds and stronger reacting opponents. The printed results belong to this run,
not to the historical leaderboard submission.


## 6. Build the submission archive

`submission.tar.gz` contains one root file, `main.py`. The check below reads it back and
verifies every byte. The source and license/credit files are also available separately
in the notebook's outputs. The agent itself needs only the Python standard library.

After a successful saved run, select `submission.tar.gz` when submitting your notebook
version. Changing settings or source creates a new policy; the historical 2615.0 does
not become evidence for that modified policy.


## 7. What the historical results tell us

| Evidence | Result | What it measures |
|---|---|---|
| S38, submission `55859195` | Historical public LB **2615.0** | One leaderboard run |
| Same code, submission `55918268` | **1833.3** in the same account check | A different leaderboard run |
| Archived local opponent panel | **38 wins / 4 losses / 6 ties** | Those 48 games only |
| Archived server replay check | **719/719 actions matched in both seats** | Reproduction of one recorded game |

The archived local panel produced the same actions with and without the second-goose
buffer. It therefore does not show that the buffer improves results. Different LB
ratings are not a controlled comparison either.

A useful next test is to compare the default with `SECOND_GOOSE_BUFFER = 0`, using the
same fresh seeds and opponents in both positions. Record how often the decision changes,
whether the animals actually get placed, and final bank differences. Keep development
games separate from the games used to evaluate the final choice.

## Credits

- [The Moon Counts Melons — prvsiyan, V29](https://www.kaggle.com/code/prvsiyan/kaggriculture-frontier-the-moon-counts-melons?scriptVersionId=345469411):
  the inherited controller, including its Ramesh-labelled public-mode schedule.
- [Kaggriculture: Getting Started](https://www.kaggle.com/code/bovard/kaggriculture-getting-started):
  environment, agent interface and submission workflow.

Released under Apache-2.0; `LICENSE.txt` and `NOTICE.txt` are included in the outputs.
AI assistance was used for implementation, testing and editing. The explanatory examples
are synthetic; the full-game check above runs the actual generated agent.
