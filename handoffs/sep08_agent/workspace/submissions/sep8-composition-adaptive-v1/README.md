# September 8 composition agent submission

Kaggle submission **56101451** is **COMPLETE**. Exactly one upload was made on
September8 at15:30:57UTC. Official validation episode106828090 finishes DONE in
both seats. Local replay reproduces all1438 submitted actions and both rewards
(66312 and69823), with no stdout/stderr errors. Server total agent time is
0.956/1.043 seconds per seat; maximum single calls0.749/0.765 seconds.
`SUBMISSION.json` and `kaggle_validation/audit.json` retain the evidence.

The selected submission is `animal_repair_q24_premium_m2`. `SELECTION.json`
records the completed evidence and known tradeoffs. The user reiterated the
outstanding upload request; completed deployment and head-to-head checks support
submitting this candidate while the larger independent league audit continues.
That audit's promotion gates remain unchanged and unpassed. The accepted
development reference remains `empty_sale_slots_m2`.

## Agent and lineage

The authoritative C++ package is retained under
`source_tree/experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/proposals/animal_repair_q24_premium_m2/`.
Its manifest and every required source dependency are frozen in `FROZEN.json`.
The generated deployment contains only `main.py`, using Python's standard library.
It has no native binary, subprocess, network, GPU or external file dependency.

The foundation is the last submitted `investment_context_guarded_001_best`
(Kaggle submission56078898): replay-derived farm schedules, optimized daily
worker schedules, milk/wool shop choices, and deferred animal investment.
Every inherited table is byte-for-byte equivalent after JSON decoding to that
submission's exported policy data. Source provenance is retained beside the
frozen components in their README and import records.

The later chain adds these components, in order:

1. `crop_value_m2_t4`: extra strawberry fertilizer when observed berry-shop demand
   reaches four. Its full crop continuation must pass the entry contract.
2. `crop_rotation_t2_berry` and `crop_mix_t2_wheat`: choose tomato or productive
   wheat rotations using shops already revealed; retain the berry suffix choice.
3. The Bohann opening and `opening_q32_b13_v1`: a wheat purchase/sale round trip
   with a 13-wheat reserve and a corresponding next-turn sale adjustment.
4. `late_value_s32_t0_r05`: rank complete farm continuations with 32 deterministic
   future-shop scenarios, whole-herd market effects and a risk penalty.
5. `wool_family_context_v2` and `rival_wool_context_v3`: guarded transfer to a
   public V52 wool schedule in useful shop contexts, broadened by public opponent
   hires, spending and inferred wheat flow. The seventh worker's opening move
   and four deferred purchases preserve the transferred schedule.
6. `observed_sale_lead_start_216` and `empty_sale_slots_m2`: project own deposits,
   anticipate the next sale, and remove empty premium-product sale orders.
   The deposit projection follows the verified Ahmed V23 public component.
7. Animal group selection: compare complete cow/sheep course alternatives with
   keeping the existing farm, including actual course labor and fixed purchases.
   Choose from observed shops and public herds; compile service schedules offline.
8. A certified day23 repair handles one exact weed38 starting state without
   another worker. Other guard misses retain the closest complete animal course
   and are explicitly audited. They are not silently treated as successful guards.
9. Prioritize positive premium-product sales from step216, and reduce the
   opening round-trip quantity from32 to24.

The C++ wrappers determine the exact override order. `export_policy.cpp` exports
their immutable tables; `policy_layers.py` and `policy_animals.py` preserve their
decisions. Unknown shops are sampled inside the estimator; actual future shops,
the environment seed and opponent-private inventories are never runtime inputs.

## Validation so far

The frozen C++ candidate wins4040/4096 games against the teammate and3883/4096
against the last submission, using official native shop/weed RNG and both seats.
Mean margins are+$11793.09 and+$3430.81, respectively. Raw results and commands
are in `FROZEN_CPP_VALIDATION.json`, `teammate4096.json` and `prior4096.json`.
Those runners have fixed C++ opponent types; their original CLI default labels
do not identify the actual compiled candidate. The retained main sources do.

The extracted Python archive passes132 official full games and94908 action
comparisons against C++. It wins63/64 against the teammate and62/64 against the
last submission. All statuses are DONE. All128 corresponding final cash pairs
also equal native C++ results. Independent-module self-play, PASS games and an
official file-path self-play are included. `packed_v1_validation.json` and
`file_runner_validation.json` contain the results and timing measurements.

`branch_validation.json` records additional C++ observation-trajectory checks,
including an activated weed repair and missed-guard continuations. The broader
league comparison remains a separate acceptance requirement.

The first Python adapter check caught a missing13-wheat opening reserve and
next-turn sale correction. It was fixed in the adapter; the C++ policy stayed
unchanged. Failed smoke and test-exporter logs are retained. Test serialization
also required explicit numeric output of byte-sized tile fields and the correct
13-field tile layout. These were offline fixture bugs, not agent changes.

## Reproduction

Run commands from the repository root, through the existing conda environment:

```text
conda run -n kaggriculture python submissions/sep8-composition-adaptive-v1/rebuild.py
conda run -n kaggriculture python submissions/sep8-composition-adaptive-v1/build_cpp.py
conda run -n kaggriculture python submissions/sep8-composition-adaptive-v1/verify_submission.py --seeds 32 --label reproduction
```

`rebuild.py` reads the retained source tree and local adapter files; it does not
read the live experiment or the previous submission. It checks identical archive,
main.py and policy-data hashes. `prepare_export.py` and `freeze_sources.py` are
retained provenance scripts from preparation, not required rebuild steps.

`validate_cpp.py` reproduces the two4096-game batches and16 debug games. Preserve
existing result files before rerunning it. `build_case_exporter.py` and
`verify_branch_cases.py` reproduce the targeted trajectory checks; the latter
uses a new output directory on its first run. Every real game has719 transitions.
Day-solver hour23 padding is never an extra action in a submitted game.

`submit_once.py` refuses to upload until selection, source hashes, exact package
checks and a deterministic rebuild pass. It creates an attempt record before
calling Kaggle and never retries automatically. `monitor_validation.py` obtains
the terminal Kaggle status, downloads its validation replay and compares every
submitted action and both final rewards locally.
