# fixed_weed_105492_robust

Observation-only solo policy for shops disabled, a `PASS` opponent, random
weeds at chance `0.005`, and both seats. Its nominal tape scores exactly
`$105492` without weeds.

The fixed runtime stack is:

1. same-turn dependency repair and exact cleanup routing, with four-day wheat
   lookahead;
2. opportunity-cost emergency funding and guarded sale-underfill substitution;
3. day-19 wheat/melon donor detours with maximum lead five;
4. exact worker reassignment on days 11, 17, and 19;
5. suppression of an excess hire only when the sole visible day-17 cleanup
   target is wheat at `(4,7)`.

The source and tape are self-contained except for the persistent agent API and
game engine. The policy has no seed access and does not predict future weeds.

Build and run the sealed smoke tests:

```text
cd tests
conda run -n kaggriculture make -j2 verify
./verify
```

See `RESULTS.md` for paired validation and known tradeoffs.

The self-contained Kaggle deployment is submission `55854973` in
`submissions/aug29-fixed-weed-105492-robust/`. Public score is `600.0`.
Validation episode `102040849` reproduces both `$97,429` rewards and every
submitted action exactly. That run includes random shops and self-play, outside
this policy's optimized no-shops/`PASS`-opponent objective.
