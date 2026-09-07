# 238/238 Known Streams | v58 Minimax Closed Loop

**If an opponent trajectory is already in our evidence, losing to it should be
treated as an engineering defect—not as unavoidable leaderboard noise.**

v58 turns that principle into a public-state best-response engine. It keeps a
strong option-preserving route, but does not require opponent identity, team
name, submission ID, hidden seed or future action access. At four observable
checkpoints it can select a compatible continuation or a narrow market timing
expert.

Headline official-engine counterfactual results:

- **58/58** against all 29 streams observed by the latest v57 submission, both seats
- **238/238** across five known regression panels, both seats
- **+10** worst margin and **+605.5** bottom-decile mean margin
- official source-file loader: `DONE` on both seats, 660 productive turns

These are **known frozen-stream development results, not a Public-LB score and
not an unseen holdout**. The scientific point is narrower: all previously known
losses can be repaired without identity leakage, and a state collision should
be solved by robust continuation value—not by adding a more specific name gate.


## 1. What failed in v57

The latest v57 run recorded 25 wins, three losses and one tie. The failures
were early enough to prevent a clean climb:

| Opponent | First / second public shop | Recorded margin |
|---|---|---:|
| Alex Paul | FARMERS / PIZZA | −1,465 |
| UncleWKTK | BAKERY / BAKERY | −1,433 |
| One Zero001 | SMOOTHIE | −858 |
| guoziyi123456 | YARN / BRUNCH | 0 |

There was no crash, malformed action or seat desynchronization. The common
opening remained valid; the continuation and market timing were wrong for
several already-observed regimes.

The repairs are deliberately sparse:

```text
step 72  : shop + public cash + opponent assets + market inventory
step 96  : two known YARN public-state regimes
step 144 : second-shop continuation where step 72 is ambiguous
step 360 : persistent exact public-farm mirror -> SELL timing expert
```


## 2. The hard part: identical public state, opposite best response

Pico, Refrain, xiongrui and edteoh share the same complete public signature at
step 72 in the ICE regime. A more complicated classifier cannot separate
identical inputs. Two obvious routes each overfit one side:

| Continuation | Pico | xiongrui | Diagnosis |
|---|---:|---:|---|
| v57 backbone | −462 | +1,429 | safe for xion, loses Pico |
| direct recovery | +3,721 | −4,781 | fixes Pico, creates a larger regression |

The correct objective is therefore minimax continuation value. I screened
state-compatible suffixes over every colliding family, selected the best
worst-regime route, then changed only one market event: a 15-unit SELL moves
from step 689 to 690.

| Frozen family | v58 margin, each seat |
|---|---:|
| Pico | +333 |
| xiongrui | +2,126 |
| Refrain | +2,104 |
| edteoh | +1,936 |

This is the most important change in v58. The router does **not** pretend it can
identify an unobservable suffix. It picks one action that remains profitable
across every known continuation compatible with the observation.


## 3. Architecture: compiled experts, sparse closed-loop control

Every expert is a complete 719-action controller with a verified shared prefix.
To stay below the one-second action budget, controllers are initialized one
per turn during steps 0–9. Each new controller first replays its missed
public prefix, so it is exactly synchronized before any branch can select it.
This prevents residual HIRE/BUY/SELL state from silently slipping after a late
route switch without paying all initialization cost on the first action.

The independent experts are:

1. default option-preserving backbone
2. capital-recovery continuation for FARMERS / BAKERY / SMOOTHIE regimes
3. second-shop continuations for FARMERS→SMOOTHIE, BAKERY→YARN and PIZZA→PET
4. step-96 YARN best response for two public capital/market states
5. ICE minimax continuation with one late SELL timing intervention
6. exact-public-mirror market expert after a 240-turn equality streak

Normal states still replay a strong offline plan. Feedback is used only when
the expected continuation changes materially. This is the practical hybrid:

```text
offline tactical memory + public-state branch + tiny market control
```


## 4. Evaluation contract: what 238/238 does and does not prove

The five panels contain 238 seat-games and all are wins. This is a strict
regression contract for *known frozen behavior*, but it is retrospective: every
stream was available during diagnosis. A frozen opponent also cannot react to
our counterfactual market action.

To catch implementation-only failures, I additionally ran dynamic games on six
new seeds:

| Pair | Games | v58 wins | Ties | v58 losses | Win value | Mean margin |
|---|---:|---:|---:|---:|---:|---:|
| v58 vs v57 | 12 | 2 | 8 | 2 | 50.0% | +998 |
| v58 vs itself | 6 | 0 | 6 | 0 | 50.0% | 0 |

So v58 preserves the predecessor dynamically; it does not establish universal
dominance. The next genuinely fresh ladder games remain the only test of an
unknown adaptive continuation.


## 5. Runtime blindness and lineage-aware research

Runtime features are limited to:

- ordered public shop unlocks
- both players' public money
- public opponent farm assets
- public market inventory
- persistent equality of the two public farm states

Explicitly absent are team/user identity, submission ID, Notebook ownership,
lineage label, hidden seed and future opponent actions.

Lineage hashes are still valuable on the **evaluation side**. Georgy Mamarin's
ladder audit shows that team- or seed-grouped validation can leak the same
turn-200 action stream across folds. v58 therefore treats hashes as corpus
provenance and collision diagnostics, never as deployment input. Public-state
aliasing is handled with a worst-family objective rather than impossible
classification.


## 6. Public provenance

The compiled route library builds on public Kaggriculture work and replay data:

- [Islet — episode 103819410](https://www.kaggle.com/competitions/kaggriculture/episodes/103819410), option-preserving backbone
- [Yukino — episode 103808285](https://www.kaggle.com/competitions/kaggriculture/episodes/103808285), compatible continuation family
- [Georgy Mamarin — Kaggriculture Episodes](https://www.kaggle.com/datasets/georgymamarin/kaggriculture-episodes), action-stream lineage methodology
- [Rayk Kretzschmar — Findings from Zero to Top Meta](https://www.kaggle.com/code/raykkretzschmar/kaggriculture-findings-from-zero-to-top-meta)
- [beicicc — C20 Exact Replication Control](https://www.kaggle.com/code/beicicc/kaggriculture-c20-exact-replication-control)
- [prvsiyan — Frontier: The Moon Counts Melons](https://www.kaggle.com/code/prvsiyan/kaggriculture-frontier-the-moon-counts-melons)

Publishing the failure matrix matters more than publishing another unexplained
route. Please preserve this provenance chain when forking, and test any repair
against the colliding families—not only the opponent it was designed to beat.


## 7. Exact artifact contract

- `main.py`: **315,484 bytes**
- SHA-256: `b041058ec187a8d0a01edc0eab8de068b53deca3e6c1973faf74ace6916ddcb9`
- deterministic `submission.tar.gz`: `90c679e02e78cb436128a6029d39c70a9f345912a1c9ff35956c5ee166b2061c`
- embedded routes: 719 × 9
- decision checkpoints: 72, 96, 144 and 360
- dependencies: Python standard library only
- official source-loader entrypoint: `kaggle_agent_v58`
- default-timeout local audit: 0.27s import, 0.23s worst warm-up action

The next cell reconstructs the exact evaluated bytes, verifies both hashes,
compiles the source and emits the Notebook-linked submission artifact.

The final Notebook cell exercises Kaggle's official source loader and the first
action in-process. Full 719-turn both-seat runs are part of the frozen local
manifest at the competition's default timeout; a nested path-runner is not used
as a proxy for the actual code-submission worker.
