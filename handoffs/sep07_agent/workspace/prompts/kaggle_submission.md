# Kaggle submissions from local C++ agents

Keep the authoritative policy as an official local agent that follows
`prompts/local_agent.md`. Kaggle ultimately runs `main.py`, so deployment is an
offline compilation step from the C++ policy into a small Python policy. Do not
put Python or Kaggle packaging inside the agent directory.

## Layout

Use this separation:

```text
agents/<agent-name>/                 # authoritative C++ agent
work/submissions/<agent-name>/
├── build_submission.py             # deterministic offline compiler
├── verify_submission.py
├── main.py                          # generated
└── submission.tar.gz               # generated
```

The archive must contain `main.py` at its root.

## Choose the cheapest deployable representation

Preserve decisions, not the C++ implementation machinery.

1. For a fixed policy, record the complete action tape from the observation-only
   C++ agent. Encode integer opcodes and quantities in fixed-width arrays,
   compress them offline, and embed the payload in `main.py`. Runtime work should
   be only step lookup, decoding, legality guards, and action formatting.
2. For a finite-state or conditional policy, compile rules into compact tables or
   a decision DAG. Keep only the observation fields used by each condition.
3. For a numeric policy, export constants and weights in contiguous arrays and
   reproduce the measured inference kernel. Benchmark plain loops and NumPy in
   the Kaggle environment; use the faster end-to-end path.
4. Hand-port arbitrary C++ search only when its decisions cannot be compiled into
   a smaller representation. Match the C++ candidate order, bounds, integer
   behavior, and deterministic budget.

Do not invoke a C++ process on every turn, serialize observations through IPC, or
reconstruct the simulator in Python for a fixed policy. Do not depend on a native
binary or extension unless the competition explicitly permits and provides a
portable build path for it.

## Offline compiler

`build_submission.py` should:

- read only the official agent's source or an observation-only replay produced by
  its test runner;
- fail fast on unexpected dimensions, opcodes, or quantities;
- generate `main.py` deterministically;
- compile-check the generated Python;
- create `submission.tar.gz` with exactly the intended root files;
- print sizes and SHA-256 hashes for both generated artifacts.

For tapes, use integer opcodes internally and convert to Kaggle action strings
only for returned actions. Decode the compressed payload once at module import,
not once per turn. Avoid dictionaries, JSON, allocation-heavy transformations,
and string searches in the hot path.

## Python entry point

Derive the tape step or policy state from public observation fields such as
`day`, `hour`, and `player`. Never rely on the number of calls made, because seat
handling and local validation may change call order.

On every call:

- return exactly one action for the farmer and each current hand;
- default missing or extra unit decisions to `PASS`;
- return at most the configured market-order limit;
- use the observed player index for the own farm;
- reset episode state at the first observed step;
- use only information allowed by `AgentObservation` in `local_agent.md`;
- keep all recovery logic bounded and deterministic.

A fixed tape must remain safe when reality diverges. Before `PLANT`,
`BUILD_COOP`, or `BUILD_PASTURE`, inspect the current tile. If it is a weed,
issue `DIG`, retain the intended action, and resume through a bounded repair
state. Cap market orders and quantities so missing workers, insufficient cash,
opponent trades, shops, or failed actions do not corrupt Python state.

## Equivalence and deployment checks

Run all Python and Kaggle commands through the `kaggriculture` conda environment.
Before upload:

1. Compile and run the official C++ agent's required checks from
   `local_agent.md`.
2. In the fixed reference configuration, compare the generated Python agent with
   the C++ agent. Require the same terminal cash and preferably the same action
   sequence.
3. Run the Python agent for a full game against `pass` with default shops and
   weeds.
4. Run full self-play and test both seats.
5. Check every final status is `DONE` with the official
   `kaggle-environments` package.
6. Inspect the tar archive and import `main.py` from a clean process.
7. Benchmark total agent-call time. Optimize only measured hot paths.

Load two independent copies of `main.py` for local self-play. Calling the same
imported module twice can incorrectly share mutable episode globals between the
two players, while Kaggle runs them independently.

Typical commands are:

```text
conda run -n kaggriculture python work/submissions/<agent-name>/build_submission.py
conda run -n kaggriculture python work/submissions/<agent-name>/verify_submission.py
conda run -n kaggriculture kaggle competitions submit kaggriculture \
  -f work/submissions/<agent-name>/submission.tar.gz \
  -m "<agent-name>"
conda run -n kaggriculture kaggle competitions submissions kaggriculture
conda run -n kaggriculture kaggle competitions episodes <submission-id>
conda run -n kaggriculture kaggle competitions replay <episode-id> -p <directory>
conda run -n kaggriculture kaggle competitions logs <episode-id> 0 -p <directory>
```

An upload is not complete until Kaggle changes it from `PENDING` to a terminal
status. Download its validation replay and require local reproduction of both
final rewards and every submitted action. A simulation public score is a skill
rating, not terminal game cash. If validation fails, keep the official C++ agent
unchanged, fix the offline compiler or adapter, regenerate, rerun all checks,
and submit the new archive.
