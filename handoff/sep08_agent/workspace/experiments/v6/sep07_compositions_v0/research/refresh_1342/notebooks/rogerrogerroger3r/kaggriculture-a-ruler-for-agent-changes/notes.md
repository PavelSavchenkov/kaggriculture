# Measuring an agent change: a paired harness, and C++ agents without root

Two things this notebook gives you, both runnable here:

1. **A paired head-to-head harness** that plays two agents against each other on the official
   engine with the seats swapped on odd seeds, and scores wins and losses only — the same thing
   the ladder scores. Plus the table that says how many games you actually need before a result
   means anything.
2. **A way to compile the C++ agents on this tab without root.** Several of the strongest public
   agents ship as `policy.cpp` + a `tape.inc` + a `ctypes` loader. If you have no compiler you
   cannot run them locally at all. `pip install ziglang` gives you a full clang toolchain in
   user space, no `sudo`, and it works inside a Kaggle notebook.

There is no agent in here and nothing to submit. It is the ruler, not the thing being measured.


## 1. Why you need a ruler that is not the leaderboard

The live rating on this competition is strongly path dependent, and the size of that path
dependence is larger than almost any improvement anyone posts.

- [Rayk Kretzschmar](https://www.kaggle.com/competitions/kaggriculture/discussion/734000)
  submitted two **byte-identical** agents two hours apart. One settled near 1,700; the other
  climbed past 3,000.
- [rivakajangu](https://www.kaggle.com/code/rivakajangu/your-rating-is-noisier-than-your-improvement)
  measured the same thing on his own account: identical file and identical hash, both past 91
  episodes, finished **268 points apart**; the same policy rebuilt and resubmitted finished
  **398 apart**.
- On this account, two submissions of a **bit-identical archive** scored 2175.3 and 2047.9 — a
  gap of 127 points, which on the leaderboard at the time was 180 places.

So a single submission cannot tell you whether a change helped. You need to measure offline, and
you need to know how much offline measurement is enough.

Two more facts that shape what you should measure, both from the hosts and from the
previous first place:

- The final ranking is **not** the live rating. Per the
  [host's post](https://www.kaggle.com/competitions/kaggriculture/discussion/731587), after the
  deadline episodes keep running for two weeks and a single **Bradley-Terry tournament** on those
  episodes produces the final leaderboard.
- Only your **two most recent submissions** are active, and the score counts **wins and losses
  only**, not the coin margin
  ([Ryo Hasegawa](https://www.kaggle.com/competitions/kaggriculture/discussion/736219)).

That last point is why the harness below scores wins and losses and ignores the size of the win.
Optimising mean final cash optimises the wrong thing.


## 2. How many games is enough

Paired games with the seats swapped remove the first-move advantage, but they do not remove
sampling error. This table is the minimum number of wins, out of `n` paired games, for the
Wilson 95% lower bound on the win rate to clear 0.50 — that is, the smallest record you can
report as "better" rather than "not distinguishable".


The last three lines are the ones worth remembering. A change that genuinely wins **55%** of
the time needs roughly **780 paired games** before you can expect to see it. A 24-game run cannot
resolve anything below about a 70% win rate, and a 24-game run that comes back 17–7 has a Wilson
interval of [0.50, 0.87] — its lower bound is sitting on the coin flip.

This is the arithmetic behind rivakajangu's point. Most posted improvements are smaller than the
noise in the instrument used to measure them.


## 3. The paired harness

Odd seeds swap the seats, so whatever first-move advantage exists cancels across the pair. Only
the sign of the final-cash difference is counted, because that is what the ladder counts.


## 4. A runnable demo

The environment ships three baseline agents — `pass`, `random` and `starter` — so this runs
anywhere with nothing attached. Point `head_to_head` at two real `main.py` files to use it for
anything serious.


On a run of this here, `starter` took all 12 games with mean final cash 3,580 against 2.
Note what the Wilson interval does even so: a 12-0 sweep only bounds the win rate below at 0.757.
A clean sweep of a small panel is weaker evidence than it feels like.


## 5. Compiling the C++ agents without root

Several strong public agents on this tab are C++: a `policy.cpp`, a large recorded `tape.inc`,
a `submission_bridge.cpp`, and a `main.py` that loads the compiled `agent.so` through `ctypes`.
Their notebooks build them with `g++`. If your machine has no compiler — and a plain WSL or a
slim container often does not — you cannot run those agents locally at all, which means you
cannot measure anything against them and are left guessing from leaderboard numbers that we
have just established are noise.

`pip install ziglang` ships a complete clang toolchain as a Python package. No `sudo`, no apt,
no system packages touched. `python -m ziglang c++` is a drop-in for `g++` for this purpose.


For a real agent the command is the one its own notebook uses, with `g++` swapped out:

```bash
python -m ziglang c++ -O3 -std=c++17 -shared -fPIC -Isource/include \
    -o agent.so source/policy.cpp submission_bridge.cpp
```

Write the author's `%%writefile` cells to disk, run that, put `agent.so` next to their `main.py`,
and the agent is now something you can play 200 games against instead of a number on a page.

One caution on where you get the sources: read them before you run them. The build cells in these
notebooks do real work on your machine. Reading a `%%writefile` cell and copying its contents out
executes nothing; running the whole notebook does.


## 6. What this does not solve

An offline panel can still mislead you, and
[rivakajangu](https://www.kaggle.com/code/rivakajangu/your-rating-is-noisier-than-your-improvement)
has the sharpest example: of three of his agents scored against his 15 hardest offline opponents,
the one that scored **best** offline, 0.933, ended up rating **worst** on the ladder. If 81% of
your practice games are decided before they start, the panel cannot rank your candidates however
many games you play.

So the harness above tells you whether A beats B. Choosing a B that resembles the field you will
actually be matched against is a separate problem, and a harder one.

---

Credit to [Rayk Kretzschmar](https://www.kaggle.com/competitions/kaggriculture/discussion/734000)
and [rivakajangu](https://www.kaggle.com/code/rivakajangu/your-rating-is-noisier-than-your-improvement)
for the identical-agent measurements, to
[Ryo Hasegawa](https://www.kaggle.com/competitions/kaggriculture/discussion/736219) for the
rating mechanics and the note that only wins and losses count, and to the hosts for
[spelling out the final evaluation](https://www.kaggle.com/competitions/kaggriculture/discussion/731587).
