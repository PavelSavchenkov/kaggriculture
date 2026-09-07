# Local C++ agents

Follow this format for every local agent. Keep policy code deployable behind a
future Kaggle adapter, but do not add Python, JSON, IPC, or Kaggle packaging here.

## Layout

Every local agent must use this directory format, regardless of where its
directory is stored:

```text
<agent-directory>/
├── agent.json
├── README.md
├── source/
│   ├── agent.hpp
│   └── agent.cpp
└── tests/                          # only when agent-specific tests are needed
```

Agent names must match `[a-z0-9_]+`. Store source and metadata in the agent
directory. Store an agent in `agents/inhouse/<agent-name>/` or
`agents/external/<agent-name>/` only when explicitly instructed; agents there
are official. Keep work-in-progress and experimental agents elsewhere, for
example under `work/agents/<agent-name>/`, so the official directories stay
small. Location does not relax any format or API requirement in this document.

Use `agents/inhouse/` only for policies with a fully recoverable local lineage.
General ideas learned from external behavior are allowed, but copied external
policy blocks, tapes, schedules, or opaque models are not. Use
`agents/external/` for public notebook ports, replay reconstructions, and other
policies whose strategic behavior is primarily external. Every external agent
must record its exact source, lineage, known or unknown approximate Kaggle
rating, parity scope, optimization status, and reuse restrictions.

Shared API, runtime, and generated build files belong only under
`agents/common/`:

```text
agents/
├── common/                         # shared code/artifacts, never agents
│   ├── api/
│   │   ├── agent_api.hpp
│   │   └── observation.hpp
│   ├── runtime/
│   │   └── observation_builder.hpp
│   └── builds/                     # generated, never authoritative
│       ├── generic/arena
│       └── pairs/<content-hash>/
├── inhouse/<official-agent-name>/  # recoverable local lineage
└── external/<official-agent-name>/ # external strategic lineage
```

Do not add top-level agent directories or compatibility symlinks under
`agents/`. Resolve official agents recursively from `agents/inhouse/` and
`agents/external/`. Use only `agents/common/api/`, `agents/common/runtime/`,
and `agents/common/builds/` for shared infrastructure.

Do not add `bin/`; binaries depend on the engine, opponent, compiler, flags, and
CPU target.

Keep generated tables, weights, and tape symbols inside the agent's namespace.
Two different agents must link into one pair runner without ODR collisions.

Use this manifest:

```json
{
  "format_version": 1,
  "name": "example_agent",
  "header": "source/agent.hpp",
  "type": "kag::agents::example_agent::Agent",
  "sources": ["source/agent.cpp"]
}
```

Keep `README.md` short: policy summary, parameters, and known assumptions.

## API

Include `agents/common/api/agent_api.hpp`. The manifest type must satisfy
`kag::agent::LocalAgent`:

```cpp
class Agent {
public:
    static kag::agent::AgentInfo info();
    void reset(const kag::agent::AgentInit& init);
    void act(
        const kag::agent::AgentObservation& observation,
        const kag::agent::DecisionBudget& budget,
        kag::Action& action
    );
};
```

Rules:

- Construct one instance per seat and match worker.
- Call `reset` once per episode and clear all episode state there.
- Call both agents on observations from the same pre-step state, then call
  `Sim::step`.
- Do not use mutable global or `thread_local` state. Immutable shared model
  weights and lookup tables are allowed.
- Separate instances must run safely in parallel.
- Do not throw across the API boundary.

## Observation and action

Use `kag::agent::runtime::make_observation`. Agents may see only:

- time and player id;
- both public farms;
- their own shed, seeds, and unit inventories;
- shared market and shops;
- public configuration from `AgentInit`.

Never expose opponent-private inventory, environment seed, future randomness,
instrumentation, full `State`, or a live match `Sim`. Build beliefs or sampled
determinizations from `AgentObservation` when search needs hidden state.

Initialize a legal fallback before search:

```cpp
action.clear();
action.n_units = 1 + observation.own_hand_count();
for (int unit = 0; unit < action.n_units; ++unit)
    action.units[unit] = kag::UnitAction{};
action.finalize();
```

The returned action must contain exactly one action per current unit, at most the
configured market-order limit, and finalized metadata. Debug runners validate
this contract; release runners may assume it.

## Search and policy hot path

Use the optimization patterns that produced measured gains in the game engine:

- Keep state, candidates, and scratch memory fixed-size and contiguous.
- Allocate reusable scratch buffers in the agent instance, not per candidate.
- Use direct typed simulator calls and cheap state copies inside branches.
- Skip forced decisions, `PASS` work, empty market work, and impossible action
  families before scoring.
- Maintain masks or compact lists for active tiles and entities instead of full
  board scans.
- Schedule the next relevant event instead of checking inactive objects every
  turn.
- Cache deterministic tables and update derived values only when their inputs
  become dirty.
- Use next-trigger counters instead of repeated division or modulo in hot loops.
- Prefer the narrowest safe count and index types.

Do not serialize, rebuild `AgentObservation`, allocate heap objects, or use string
lookups inside search nodes. Keep candidate order deterministic.

Let the compiler auto-vectorize first. The engine found explicit AVX2 neutral and
wider counts slightly slower, so require a controlled benchmark before adding
explicit SIMD or wider storage. Do not use `-ffast-math`.

## Inference-optimized agents

The deployable inference path must be CPU-first. GPU use is suitable for offline
training, not required agent execution.

- Load or initialize immutable weights once; share them read-only across agent
  instances.
- Store weights and features in aligned contiguous arrays.
- Prefer small framework-free C++ inference kernels for MLPs or linear models.
- Use `float` unless a measured quality requirement needs `double`.
- Extract features once per search state. Update incremental features from dirty
  tiles, inventory items, or market products when possible.
- Score candidates in batches so feature and model loops can vectorize.
- Prune with legality and a cheap heuristic before expensive inference.
- Fuse normalization, linear layers, and activation passes when it reduces
  memory traffic.
- Quantize only after measuring both decision quality and end-to-end throughput.
- Reuse activation and candidate buffers; do not allocate tensors per call.

Profile the full decision path. Faster standalone inference is irrelevant if
feature extraction, state copying, or candidate generation dominates.

## Budgets

Policies must be anytime: keep the best fully evaluated action and stop safely.

- Use `max_expansions` for deterministic comparisons.
- Use soft and hard deadlines for deployment-like tests.
- Check time periodically, not at every node.
- Do not start expensive work after the soft deadline.
- Always return the fallback before the hard deadline.

## Match runners

Use two modes:

1. Generic arena: runtime agent selection for smoke tests and broad screening.
2. Pair runner: concrete `run_pair_batch<AgentA, AgentB>` compiled with the
   engine, both agents, LTO, and release flags.

Build pair runners lazily. Cache keys must include both agent directories, common
API and runner sources, engine headers, compiler version, flags, build mode, and
CPU target. Canonicalize the pair; one binary handles both seat assignments.

Do not compile or launch per game. One invocation runs the whole seed batch:

```text
pair_runner \
  --games <count> \
  --seed-start <uint64> \
  --seat-mode both \
  --threads <count> \
  --budget-expansions <count> \
  --output <results.json>
```

Also support a seed file. Each worker owns its `Sim` and two agent instances.
Keep job assignment deterministic. Buffer results and avoid per-game stdout or
traces during throughput runs.

Use the generic arena unless pair specialization shows a material measured gain.
Do not eagerly build every possible pair.

## Build

Run builds through the `kaggriculture` conda environment. Local release flags:

```text
-std=c++20 -O3 -march=native -mtune=native -DNDEBUG
-fno-exceptions -fno-rtti -fno-math-errno
-fno-semantic-interposition -fno-plt -flto
```

Train PGO on the actual agent pair, candidate distribution, search depth, and
inference workload. Do not train it only on recorded engine transitions.

## Required checks

1. Compile in the generic arena and one pair runner.
2. Run full self-play and a full match against a `PASS` agent.
3. Validate every action in a debug build.
4. Verify independent instances share no mutable episode state.
5. Verify deterministic results under a node budget.
6. Confirm the observation excludes hidden state and seed.
7. Confirm generic and pair runners produce identical actions and rewards for
   identical seeds and budgets.
8. Benchmark throughput changes before keeping an optimization.
