work for 24h in experiments/v6/sep07\_compositions\_v0/



The ideas is to develop the best agent roughly using the following ideas (my wording):



my intuition&#x20;

- strategy composition (thats what I call { (product, count, day start, day end) } i.e. cow 1 lives from day 5 to day 28, wheat 2 lives from day 1 to day 20, etc) should be generally enough to judge strategy strength
- having 2 compositions OR composition vs agent, it might be fast to have a rough estimation of how good our composition is. Roughly like this
  - simulate our composition from day 1 to day 30
  - place products on tiles greedily
  - for each day, we need to decide on sales and purchases (again, approximate). The key idea is that our aim to water all crops, collect all fertilizer, feed+care (when its due) for all animals, etc. Based on top players replay, thenn maximise output from from crops and animals they have. You can have a fast estimator for number of workers you need, number of wheat/fertilizer you need to purchase and when earliest you can sell. That might be enough to judge general strategy strength (before we have exact worker schedules).
- for a fixed composition, we can run local optimization (i.e. swapping 2 tiles will preserve rough strategy strength, but will change final profit a little bit) and then actual day-to-day workers schedule. On top of that, obviously again run some local optimization for moving sales/purchases within one day depending on opponent state and how much money you actually have

1. Top players seem to be making decisions on the fly and placing crops/animals on tiles dynamically (e.g. sometimes animal is far away from center and you have to pass through crops to get to it)
2. The key idea is that in order to make a decision like "what is better to buy now, nothing, cow, or sheep" you do not have to simulate all future game, up to worker movements, it might be enough to look at some rough formulas like "how much you gonna spend on it per day, how much it will produce, etc" which should be almost instantenious

in other words (again, a bit messy, but summarise into coherent strategy, based on my intuition and what top players (judging from replays) *seem* to be doing and what *seem* to work judging from top players):

- outer search is over compositions
- composition strength can be estimated very quickly -- greedy tiles placement (greedy is a vague word, of course we can optimize placement logic while keeping it fast -- again, use top replays as inspiration), evaluation if composition is possible at all, assumption that we following general best practices from top players (i.e. try to get most of each animal/crop), quick estimations of how many workers needed for the day, at which hours we need to sell/buy, which products we can bring to shed earlier, etc (again, try to make it faster, so some minimax or other types of searches can investigate the decision tree deeper, without actually solving day schedule exactly every time, or just reusing some day schedule templates -- whatever works to estimate day scheddule accurate enough to estimate strategy strength)
- when we estimated things , we can actually convert composition into a concrete strategy by applying&#x20;
  - day solver
  - local optimizations -- we can move some sells/fertilizer/etc a bit earlier or later and see what works best in local sense
- you can freely use branching depending on shops of course



You should also develop an actual strategy improvement loop. Maybe its enough to keep coming up with betteer and better strategies using self play and some gradually increasing league. You can use anything in the league -- most importantly best agents from my teammates (see external/\*), best agents pulled from kaggle (we had some strong agents converted to c++, search the repo), and agents from previous iterations of this session.



Improvement loop I see is smth like that: take some agent, we want to develop a strategy that beats it. Start with some good opening and come up with the first composition proposal (of course its better to take good proposal, not random), then start "playing" against this opponent agent in the "estimation" sense -- do not develop strategy up to exact workers movements, only estimate how good this compositioin or branching into different composition suffixes depending on shops will perform. I.e. its probably best to run this simulated game for many times so we see how our proposal performs in many shop scenarios. Then implement proposal into concereete strategy and re-test. Use "complete strategy" vs "estimated strategy strength" as a signal for improving complete straategy compile and estimation engine.&#x20;



Super impoortant source of ideas and patterns for everything is a dataset of replays of top players. Pull the latest onees, keep looking at them for clues.



The final goal is a strong (branching depending on shops and/or opponent, if helpful) agent which beats our teammates best agent, best pulled from kaggle agents, and its own previous iterations.&#x20;



If any part of the pipeline is slow, think if we can re-design the pipeline, or just optimize (both in cpp implementation sense and logical sense - e.g. maybe some routes or braches are clearly bad from the start), or re-design part of the pipeline, etc



Above is my stream of thoughts, compile into the most coherent, reasonable and efficient goal phrasing which preserves as much semantics that I've communicated as possible and keeps working towards stated goal of a strong agent which keep improving by re-optimizing against league (including previous versions of itself) and keep learning from top replays.



Everey 20 minutes, look at the progress, decide if we need to reprioritise ideas ledger or pivot.