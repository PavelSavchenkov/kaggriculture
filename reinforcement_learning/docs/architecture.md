# Architecture and learning contract

## Problem

Kaggriculture is treated as a finite-horizon, simultaneous-action, partially
observable zero-sum game. There are 720 recorded states and 719 action decisions.
The transition function is known; uncertainty at deployment comes from the other
policy, opponent-private inventory, and the hidden episode seed.

For an opponent distribution `mu`, the training objective is

```text
J(pi; mu) = E[win + 0.5 * draw]
```

Bulk training approximates a robust response to a league mixture. LocalLB field
performance and direct performance against the strongest agents are reported
separately because Elo against a heterogeneous field can conceal a bad top-agent
matchup.

## Information boundary

The actor receives public state for both farms plus its own private state. It does
not receive the opponent shed, opponent worker inventory, opponent seeds, engine
seed, RNG state, or opponent checkpoint identity. Unknown values require explicit
masks; zero is a real inventory value and cannot mean unknown.

Own worker inventories include both per-item counts and 1-based dictionary
insertion ranks. The rank is actor-visible and affects transition semantics:
capacity-limited DROP operations consume items in insertion order. Omitting it can
turn a legal replay sale into an impossible target and can change deployed plans.

A separate centralized critic may consume full simulator state and opponent
lineage during training. Actor and critic encoders remain separate so privileged
features cannot leak through shared activations.

## Actor

The checked-in v2 actor is a reduced hybrid sized for the available clone data:

1. shared residual CNN over each 10x10 farm;
2. tile, unit (including inventory counts and insertion ranks), market, shop, and
   global entity tokens;
3. 24 Perceiver latents, width 192, four self-attention blocks, six heads;
4. a 192-wide hourly GRU and 192-wide daily GRU;
5. parallel worker heads and a ten-step autoregressive market GRU;
6. W/D/L, margin, and opponent-flow heads.

The actor is canonicalized to the acting player's perspective but retains a seat
bit because end-of-day RNG consumption and market commit order create measurable
seat asymmetry.

The simpler required baseline is a small CNN+GRU with the identical tokenizer,
masks, action sampler, and losses. The transformer survives only if it improves
closed-loop outcome or learning efficiency enough to justify deployment cost.

Initial smoke measurement on this host, batch 32 and eager FP32: 2.29 ms on the
RTX PRO 6000 Blackwell and 37.3 ms on CPU. These are interface smoke numbers, not
the final throughput benchmark; BF16, compilation, action decoding, recurrent
rollout transfer, and batch-size sweeps still belong to M0.

## Action distribution

The action is structured, not a softmax over complete joint actions.

- Each active unit predicts `op`, conditionally meaningful `item`, and exact
  quantity. Network evaluation is parallel; exact masks consume shared seed and
  shed budgets in a cheap sequential tensor pass.
- Market lines are decoded autoregressively because line order controls same-turn
  financing and shared quotes. A decoder-only `END` terminates the sequence.
- Quantity supports exact meaningful counts and semantic `ALL_AVAILABLE` /
  `MAX_AFFORDABLE` choices.

Log probability must describe the action actually sent to the engine. Therefore
sampling, masking, canonicalization, and PPO log-probability calculation must use
one action-codec implementation.

## Replay pretraining

Replay samples are `(actor_history_t, actor_observation_t, policy_action_t, z)`.
The codec records the action on the state it produced, so the correct label at
state `t` is `episode.actions[t+1]`.

Raw requested actions are not stable labels. Unit targets use accepted canonical
quantities; market targets use the PASS-opponent sanitizer and compact successful
orders. This excludes outcomes that depend on an unseen simultaneous opponent
order while retaining executable policy behavior.

Submission IDs are used when present. Older corpus files are split by normalized
early/mid/late unit, market, and item-action fingerprints.
A shared backbone plus cluster adapters supplies statistical strength; validated
clusters are then materialized as independent league lineages.

## Online learning

The first optimizer is recurrent PPO/MAPPO with `gamma=1`. With terminal-only
reward and fixed horizon, discounting would not change the mathematical optimum,
but `gamma=1` avoids shrinking an already sparse signal. Cash delta, net worth,
production, and final margin are auxiliary losses only.

Opponent sampling combines:

- current and historical checkpoints;
- frozen behavioral-clone ancestors;
- specialists retained for novelty;
- explicit best responses;
- periodic matches against the real LocalLB field.

PFSP competitive and exploiter weights are mixed with a novelty floor. Promotion
uses common seeds, both seats, confidence intervals, full-field score, and direct
top-agent score.

## Engine-guided improvement

After the actor and value are calibrated, sampled candidate actions can be advanced
through exact engine copies and bootstrapped with the observation-only value head.
Search must sample hidden state from a belief and opponent actions from league
policies; giving search the true hidden seed or opponent inventory would create an
undeployable oracle.
