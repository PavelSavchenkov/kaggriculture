Most agent notebooks in this competition are one cell: here is the code, good luck. I have published a good few here that are the opposite, all explanation and no agent, so this one closes the gap. It is a complete, working, submittable agent, and every layer in it is explained: what problem it solves, why it works, how I described it, and how I tested it.

**Why these layers and not others.** I did not pick them because they are the cleverest things I have built. I picked them because **none of them is tied to the chassis underneath**. Each one reads the observation, looks at the action the base was about to emit, and either lets it through or edits it. Drop them onto a different base, a rules agent, a tape, somebody else's public notebook, and they still make sense. They are worth more to you as techniques than as code, and section 3 is the one I would take.

**What is published here.** The agent below is a chassis plus two layers of mine: a mirror counter and the financing repair. A third of mine, a sell-schedule refinement, runs on a different chassis and is not in this file; section 6 explains it anyway, because the reason it does not travel is the useful part. The chassis is somebody else's public work, credited and unmodified. On top sits a thin decision layer of mine that fires on about half a turn per game and is otherwise invisible.

Its immediate predecessor, which is this same chassis and the same mirror layer without the financing one, **read 2,593.8 over 203 episodes on the day I retired it** to make room for this version. I retired it, it did not fall off. Two caveats I would want if you were quoting a number at me: a rating here is a reading at a moment and not a property, since the field turns over with a half life I measure at about two days, and that submission was live rather than frozen when I read it. The agent published below plays the identical game on that population: over its predecessor's own 204 recorded episodes the two won and lost **exactly the same games**, 0 discordant.

**Why I am publishing it.** The interesting part is not the score, it is one defect and its repair. An agent on this ladder can finish an episode DONE, emit a well-formed action on all 720 turns, never crash, never time out, and bank exactly zero dollars. I measured that happening in **50 of 179** real ladder episodes of one of my own submissions, and the rate was **rising**, from 18 % of games in the oldest quarter of its record to **47.7 %** in the newest. Nothing in the leaderboard, the logs or the episode viewer tells you it is happening. If you are running a taped agent on this board, it may be happening to you right now, and section 3 shows how to check in a couple of minutes.

The repair costs four dollars.

Everything below is measured. Where a number appears, the command that produced it is named.


<a id="s1"></a>
## 1. A chassis plus a thin layer

There are four roads to an agent here and I have written about them [before](https://www.kaggle.com/code/destbreso/island-ga-an-owned-schedule-is-a-moat). This one takes the road that is cheapest to reason about: take a strong public base, leave it completely alone, and add a decision layer that only speaks when it has something to say.

The shape is:

    action[t] = layer(state[t], chassis_action[t])   if the layer fires
    action[t] = chassis_action[t]                    otherwise

That second line is the whole design, and it buys one thing that matters more than any measurement: **with the layer switched off, the agent is byte-identical to the chassis.** Its worst case is a tie with the thing it is built on, and its only burden is that the times it does fire are right.

I do not assert it, I test it, before anything else. With `L_FINANCE = 0` I compare the emitted action dictionary against the parent on **every turn of three episodes**: **0 differing turns out of 2,157**. If that number is not zero, nothing measured afterwards means anything, because I would be comparing two different agents and attributing the difference to the layer.

I recommend the pattern for one job in particular: making a structural improvement that you want to keep **incremental and separable**. Each layer is one hypothesis with its own switch, its own free control and its own measurement, so you can accept one and reject the next without re-deriving anything, and a layer that turns out to be wrong is contained to the turns it fired on.

It is not always the best way to improve an agent, and I am not claiming it is. Searching a parameter space whose parameters interact is a perfectly valid road, and it finds things a stack of independent layers never will, precisely because it can move several coupled quantities together. It is also much more expensive, and it is not what I am doing here. What this approach buys is cheapness: the control costs nothing, and the blast radius of a mistake is one layer.

Within that frame, the default is the whole game. A layer whose default is its own policy has to beat the chassis everywhere at once. A layer whose default is the chassis only has to be right where it fires.

**And it is why these layers port.** Nothing in them knows what the base is. They take the observation and the action the base proposed, and return an action. That is the entire interface, so the same code sits on a rules agent, a recorded tape, a search-compiled schedule or somebody else's notebook without a line changing. The interface is the portable part; my code is just one filling of it.


<a id="s2"></a>
## 2. Credit, and the licence

The chassis is **[three-day-shop-router](https://www.kaggle.com/code/yhay81/three-day-shop-router) by Yusuke Hayashi (yhay81)**, used verbatim under **Apache-2.0**. It is a native C++ policy: 72-turn segments aligned to the shop unlock clock, two tapes that differ only inside one of those blocks, a route decision taken at t=360 on public state, and a six-day budget guard. The notebook below rebuilds it from his sources unmodified and prints their SHA-256 so you can check that nothing of mine leaked into his half.

The shop clock the chassis is built around is an engine invariant, not a fitted fact: shops unlock at `t = 72k`, exactly every three days, exactly eight of them. So information arrives on a block boundary and never inside one, which is why segmenting on 72 turns is the right unit and why his design is worth building on.

Nothing in the C++ is mine. Sections 3 to 6 are.

I also owe a second kind of credit, to people who never handed me anything on purpose. Every measurement below is taken against **recorded episodes of real opponents** from the public episode API: their action streams, at their seeds, from the seats they actually played. The diagnosis in section 3 exists because those rivals played the way they did. Names appear in the raw pools rather than here, and no opponent's code was used or copied, only the public record of games they played against me.


<a id="s3"></a>
## 3. An agent that banks exactly zero, and never says so

### The symptom

A taped agent finishes the season with **\$0.00**. It did not crash. It did not time out. Its status is DONE, it emitted a distinct, well-formed action on essentially every one of the 720 turns, and the rival banked 100k-160k in the same game.

**And this is not a broken toy.** The submission it happened to was rated **2,214.8** on the day I measured it, competing normally, winning the games it did not throw away. It was scoring above 2,200 while losing roughly one game in four to something that never announced itself. That is the reason this was worth a day: a defect an agent survives is a ceiling rather than a cause, and the headroom under it is whatever the dead games would have been worth.

In one of my live submissions this happened in **50 of 179 episodes**, and it got worse as the run went on:

| quarter of the live record, oldest first | rival's median bank | games banking zero |
|---|---|---|
| 1 | 80,698 | 18.2 % |
| 2 | 99,976 | 15.9 % |
| 3 | 108,274 | 27.3 % |
| **4, newest** | **109,998** | **47.7 %** |

The failure rate tracks how strong the opponent is. That is the first clue and it is the one that explains everything else.

### The diagnosis, and it is four dollars

I separated the dead games from the survivors of the same agent on the same population. One column separates them cleanly:

| | dead games | survivors |
|---|---|---|
| **money at the end of day 0** | **0** | **29** |
| hands on day 1 | 0 | 3 |
| refused hire orders | 255 | 0 |
| opponent's market SELL orders, days 0-2 | **16** | **6** |

Then I stepped through one dead episode turn by turn:

* **t1.** The opening buys 53 wheat and sells 48 of it to fund two cows, two sheep, five hires and its seed. Money goes 3,000 to 1,360 to 0. Every purchase succeeds. It ends day 0 with **exactly nothing**.
* **t24, day 1, hour 0.** Hands do not persist overnight. The engine clears them at the daily reset, so the tape re-hires each morning. It emits three HIRE orders. The engine prices the nth hire of the day at `fib(n)`, so those three cost **\$4**.
* It holds **\$0** and **3 WHEAT in the shed, quoted at \$28 each**.

It cannot pay four dollars while holding eighty-four dollars of wheat. The hires are refused, silently, because this engine has no error channel: an unaffordable order simply does not happen. With no hands nothing is worked, nothing is produced, nothing is sold, and the farm sits at zero for 29 more days.

**Whether the tape ends day 0 on \$29 or on \$0 is decided by the opponent.** The opening sells 48 wheat into a shared market. A rival who sells hard on days 0 to 2, sixteen sell orders against six, moves the price we receive. The tape was built to spend to its last dollar, so a few dollars of price impact is the difference between a season and nothing. That is why the failure rate follows the rival's strength, and why it got worse as the ladder moved me into harder company.

### Why the obvious fix is the wrong fix

The obvious repair is a cash reserve: refuse to spend below some floor. I measured reserves of 5, 10, 20 and 40 dollars, over the first 24 turns and the first 72.

**Every one of them removes all the zeros and takes wins from 52 of 77 to 15.**

The reason matters to anyone running a tape. **A reserve changes what the tape buys, and a tape is a fixed sequence whose later turns assume the earlier purchases happened.** Cancel one seed purchase on day 0 and the plantings that were scheduled for it never happen, the harvest that was scheduled for those never arrives, and the whole season detaches. A reserve of five dollars does that just as thoroughly as a reserve of forty.

I have a second, independent measurement of the same effect. Replaying a recorded tape at its own seed against a *different* opponent, a strong adaptive agent's tape banks a median of **0.78** of what it originally banked, and the mechanism is visible turn by turn: its cash trajectory desynchronises, its hires fail, its hands go missing, and the rest of its choreography addresses tiles nobody prepared.

### The repair, which changes the ORDER and not the contents

The market list is **ordered**, and the engine settles it by index: order 0 resolves before order 1. So a SELL at a low index funds a BUY at a higher index **within the same turn**.

So the layer is: on any turn whose market list contains a HIRE this farm cannot fund, prepend a SELL at index 0 that covers the shortfall from the shed.

It adds no purchase and removes none. It cannot desynchronise the tape, because the tape's purchases are exactly what they were, in exactly the same order, merely funded.

It is fail-closed in three places, because a wrong guess here is invisible:

* an observation missing a field it needs, return the action untouched;
* a market list already at the engine's ten-order cap, return untouched, because the cap silently drops the tail and displacing an order is the reserve's failure mode wearing a different hat;
* a shed with nothing priced above zero, return untouched.

**This is not my idea, it is the field's.** I measured the strongest agent I could find doing exactly this on **2,280 of its 3,624 buy-turns, 62.9 %**, while my own agent did it **0 times in 5,457**. Chained financing is standard practice at the top of this board and it is nearly free.


<a id="s4"></a>
## 4. What it is worth

Measured on the **179 episodes of that submission's own uncapped pool**, captured the day of the measurement, replayed at each episode's recorded seed from its recorded seat against that episode's real opponent stream.

The identity check comes first, because a harness that cannot reproduce the recorded game cannot measure a change to it: the unrepaired agent reproduces **178 of 179 recorded banks to the dollar**, and its recorded 49.7 % win rate exactly.

| | unrepaired | repaired |
|---|---|---|
| games banking exactly zero | **50 of 179** | **0** |
| wins | 89 | **91** |
| median bank in those 50 games | 0 | **76,976** |

The failure fires against rivals banking a median of **158,152**. Those are games I was losing anyway. So recovering 77,000 dollars of bank in fifty games flips exactly **one** of them into a win. The repair is worth about **two wins in 179**, not a leap up the leaderboard.

I am shipping it anyway, and the reason is a rule rather than a hope: **a structural change that fixes a measured defect is not judged by the size of its number.** An agent that throws away a quarter of its games is broken whatever the rating says, the rate was getting worse rather than better, and a dead game also contributes nothing to any measurement I take from the ladder afterwards. The repair costs nothing: zero wins lost, the median bank up, and it fires half a turn per game.

If your own tape spends to its last dollar in the opening, this is worth two minutes of your time. Replay your recorded episodes and count the ones that end at exactly zero.


<a id="s5"></a>
## 5. When the opponent is running your own chassis

`L_MFR` is a mirror counter. When the opponent is running the identical public chassis, which happens because a lot of us are, the two agents emit the same tape and the market becomes a race to sell first into the same book. The layer detects the mirror from the opponent's own emitted orders, requires a run of agreement before it believes it, and then front-runs the tape's own next-turn non-wheat sells.

How I tested it, on this chassis and not on the one it was first built for: self-mirror over all **64 shop worlds in both seats**, 128 games. The detector fired in **126**, and in every one of those the layer won: **126 of 126**, mean **+3,129**, median +3,173, worst case still **+1,105**. Route identified 126 of 126.

The two games where it did not fire are the interesting ones. They are the same seed from both seats, the play is byte-identical, and the margins are -8,655 and +8,655: that world's own seat asymmetry, with the layer correctly silent. A detector that fires when it should not is worse than no detector, so both failure directions get their own gate: with the layer off the agent is byte-identical to the bare chassis (12 of 12 games, zero differing turns), and with it on against three non-mirror donors it stays byte-identical too (12 of 12, zero diffs). No mirror, no edit.

The idea is not novel and I am not claiming it.


<a id="s6"></a>
## 6. A bonus layer that is NOT in this agent, and why

The third layer I run is `L_EDIT`, and it is deliberately absent here. It is worth a section anyway, because the reason it does not travel is a property of the technique and the way it nearly fooled me is the most useful thing on this page.

**What it does.** A recorded tape sells on a fixed schedule. `L_EDIT` rewrites individual sell orders of that schedule: scale a quantity up, or shift an order a turn or two earlier or later. Nothing else. It matches by **exact order signature**, the triple (SELL, item, quantity) at a named turn, and if the signature is not there that edit is skipped.

**Why it is not in this file.** That signature is the whole mechanism, and it binds the layer to ONE tape. The table says "at turn 413, find a SELL of 10 FERTILIZER and make it 20". The chassis published here emits a different tape with different sells at different turns, so every edit would miss and the layer would be an elaborate no-op. The other two layers read the observation and edit whatever action was proposed; this one reads a table. **That is exactly the criterion I used to choose what to publish**: the two that port are here, the one that does not is described instead.

**When it is worth building.** You have a fixed plan you are happy with, you do not want to regenerate it, and you suspect the selling is the loose part. The plan stays untouched and the search space is small: one edit per variant, then stack the ones that survive.

**How to accept one, and this is the part I would copy.**

1. **Fail closed.** An absent signature skips the edit. An edit late in the season additionally requires the route it was measured on to be the active one. A layer that guesses is worse than no layer.
2. **Screen and confirm on DISJOINT slices.** Mine screened on corpus routes 0-33 over 12 world-seeds in both seats, 816 games, and confirmed on routes 34-67, which the screen never touched. Screen +1,040 a game with 816 better and 0 worse; confirm +862 with 812 better and 4 worse. Selecting the best of N and reporting its own score is inflated by 24 to 27 points here.
3. **And the gate the first two do not give you.** The full table scored +862 a game against recorded routes and then ranked BELOW the unedited agent against live, reacting opponents. One edit carried the whole reversal: shifting a single one-dollar FERTILIZER sale from t157 to t155. Worth **+244** against recordings, 478 better and 2 worse. Worth **-20,672** against a reactive family, flipping 49 of 128 games.

The chain says why, and it is the same cash knife-edge as section 3. That tape's cash reaches exactly zero at t170. Move a sale two turns earlier and the money arrives before a purchase instead of after it, the herd ends t200 at seven cows instead of eight, and an animal the tape had funded silently fails to buy.

**A recorded opponent cannot move prices.** So a corpus of recordings, however large, cannot price an edit that spends cash slack. Mine could not, and it would have shipped an edit that costs twenty thousand a game against anyone who reacts.

The surviving subset is restricted to t >= 400, past the cash-tight window, and it measures 128 of 128 against that same reactive family at +25,370 mean against the unedited control's +24,903. Small, and real, and the six edits I threw away were the ones that mattered.


<a id="s7"></a>
## 7. Building it

Three steps: write the chassis sources, compile them, write the agent. The SHA-256 of every source is printed so you can verify the chassis is yhay81's, unmodified.

**The compile takes about four minutes here and it has not hung.** Measured on this image: 216 seconds of the run's 243 is that one `g++` line, and the same command on my laptop takes 0.6 seconds with a different compiler, so I am reporting the cost rather than explaining it. I keep `-O3` because it is the recipe the binary I actually field was built with, and reproducing that recipe is the point of the hashes above. If you are iterating on your own layer rather than on the chassis, drop to `-O1`: the agent uses 0.22 ms of its 1,000 ms turn budget, so nothing here is optimisation-bound.


<a id="s8"></a>
## 8. The submission

This writes `submission.tar.gz` holding `main.py` and the compiled `agent.so`. That is the artefact this competition takes for a native-chassis agent: a bare `main.py` would load nothing and score nothing, which is a mistake worth not making once.


<a id="s9"></a>
## 9. The gates I run before spending a slot

Five submissions a day and only the last two receive episodes, so a slot is the scarce resource, not compute. Everything here runs offline before one is spent.

1. **The control.** Layer off, action dict compared against the parent every turn over three episodes. **Zero differing turns or stop.** This has caught a shadowed variable that rewired a whole farm and a refactor that changed 1,282 of 1,438 turns, neither of which failed loudly.
2. **The entry point.** Kaggle runs the **last callable bound at module level**, not the one named `agent`. I measured a layered agent for an afternoon through `mod.agent` and was measuring the bare chassis with none of my layers in it. If you stack layers, check which function is actually being called.
3. **Robustness.** Every episode must finish DONE with a real reward and a well-formed action every turn, across several opponents, both seats. Plus a degraded-observation probe: remove one observation field at a time and replay, because an agent that silently collapses on a missing field does not crash, it just stops playing. Mine holds on all eight fields.
4. **The failure population.** A gate is only a gate against a population that can produce the failure. My zero-bank check runs against the exact episodes where the failure was observed, and it is validated **both ways**: it fails the broken agent and passes the healthy ones. A check that only ever fails broken things has not been tested.
5. **Paired, on real opponents.** Never a raw win rate against a panel you chose. Replay recorded ladder opponents at their own seeds, both seats, and read the **discordant** games, the ones where exactly one of the two agents won. That count is the real sample size, not the number of games.

An offline instrument of mine once read 72 % where the ladder read 13 %. The population I was scoring against was not the population the ladder pairs me with, and no amount of games fixes that. Score against the field that pays.


<a id="s10"></a>
## 10. What is here and what is not

Here: the chassis in full with its credit, the financing layer with its diagnosis and its numbers including the unflattering one, the mirror counter, a third layer described but not shipped, and the gate procedure.

Not here: the schedule search and the route-selection work, which I have written about [separately](https://www.kaggle.com/code/destbreso/island-ga-an-owned-schedule-is-a-moat) without publishing the artefacts. That is a deliberate line and section 8 of that notebook argues it.

If you clone this, the piece worth taking is not the tape. It is section 3: **look for games that end at exactly zero, and check whether your opening spends to its last dollar.** The engine will not tell you.

My other notebooks, if the measurement side is what interests you: [X-ray your agent](https://www.kaggle.com/code/destbreso/x-ray-your-agent), [A DNA test for agents](https://www.kaggle.com/code/destbreso/a-dna-test-for-agents), [Dissecting the top two](https://www.kaggle.com/code/destbreso/dissecting-the-top-two), [Wins, not money](https://www.kaggle.com/code/destbreso/wins-not-money).
