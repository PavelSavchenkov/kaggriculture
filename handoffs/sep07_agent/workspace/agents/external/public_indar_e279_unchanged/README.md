# Indar E279 unchanged public source

This directory freezes and mechanically ports the selected representative of
the public Tetsutani/Flexonafft/Indar BL-MDgogo/Kawashigi family.

- Public notebook: `indarkarhana/rank-top10-read-the-market-choose-the-farm`.
- URL: https://www.kaggle.com/code/indarkarhana/rank-top10-read-the-market-choose-the-farm
- Notebook SHA-256: `f0c7ccf2781f9287e5728e737e6ef3f72d047f66fa8dfeb33843146842ce1edd`.
- Frozen Python SHA-256: `d39dba50793d9777c990347443bf0c481c78adaea86055f6f6b0600dcfcd9f2e`.
- Python version label: `E279-V17-demand-dominance-MoE`.

`scripts/public_extract_indar.py` verifies the frozen source and clean static
audit before decoding the four literal base85/zlib/JSON payloads without
executing the source. It emits canonical JSON and a compile-time-gated C++
header. The C++ bridge preserves the low/high action selector, reconstructed
market curves, weed replay, market ranking, R5/MD counters, late room guards,
terminal liquidation, and effective live-hand normalization.
The final `action.finalize()` call supplies evaluator-only order metadata; it
does not change the ordered unit or market decisions compared by parity.

`artifacts/public_indar_parity_report.json` is the parity authority. It covers
both seats and both selector experts in complete normal-weed self-play
trajectories, plus explicit both-seat pad/truncate hand fixtures.

The claim is limited to reached normal-game observations. Malformed input,
nonstandard board/configuration values, the Python broad-exception fallback,
and arbitrary off-trajectory state histories are not claimed equivalent.
This is an unchanged source port and has no mechanism switches or adaptation.

Tetsutani, Flexonafft, and Indar are permanently one
`bl_mdgogo_kawashigi` lineage. They cannot contribute independent train/holdout
membership, cluster weight, or phenotype votes.

The reusable C++ implementation replaces the bounded sell-ranking vector with
a fixed array. This removes per-decision heap work without changing any ordered
action over all 4,096 shop sequences and both seats. Runtime is already near
the simulator floor, so no material end-to-end speedup is claimed.
