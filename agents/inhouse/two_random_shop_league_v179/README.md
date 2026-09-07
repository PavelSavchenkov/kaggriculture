# two_random_shop_league_v179

Self-contained Stage-3 two-random-shop agent frozen from experiment variant
v179. It vendors its Stage-1 base under a private namespace and has no runtime
or source dependency on the experiment or league opponents.

The policy uses only ordinary observations. It combines the Pizza-to-Ice base,
Yarn step-96 transient Pet continuation, Yarn>Smoothie target, Yarn Wheat
stagger, capacity repair, and observation-guarded terminal purchase repairs.

`agent.json` is the source manifest. `make strict` builds the reset/determinism
test with warnings as errors; `make sanitize` builds the same test under
ASan+UBSan.

The untouched 262,144-game league audit measured `J_comp=0.710013057`. The
full 262,144-game PASS regression measured `J_cash=130128.602289`, above the
Stage-3 floor. Strict, sanitizer, reset, release, manifest-closure, and 1,024
game experiment-to-seal action-equivalence checks pass.

The aggregate SHA-256 of `agent.json` and all files under `source/` is
`994430eb727438e0e789483a57ffcf55253a67edd38022c5e7f5f4dfe4db0382`.
