# pavel-bc-opus-v17d-dc12m3-d3crop-m68 (Kaggle 56690263, submitted 2026-09-29 23:46 UTC, user-asked)

d3crop-m68 (Local-LB branch pavel-bc-opus-v17d-dc11v62-d3crop-m68; all 24 model files byte-identical) with the Day compiler's dc12 m3
keys in model/model.bin.dc11: m1 (dropany=6 cashsell=1 wheatcash=1 reserve=0 saleslots=3) + nighttrim=1 + survivalfloor=1.
Package built by BC (experiments/v10/sep29_bc_mm/packages/d3m68_m3, tree bc_m3min); source/ and standalone/ hold the bridge sources.

Local evidence (vs d3crop-m68 unless noted): league vs the 3 strongest local agents +1.50k (SE 0.53k), level vs the other 5; pinned 200
real games (field recorded opponents) m1 trimmed +1.5k; Weaknesses' 760 clone games +1.1k; contested clones +0.9k; wide bed -1.24k;
seat-swap bed (234 exact live games, top team's seat) ~-1.2k vs top-10 seats (our lineage earns more). survivalfloor: 0 changed games in
72 league / 200 pinned; fixes the m1 idle-day collapse (akmr game -182k -> -7.9k). landfirst NOT included.
Checks: local verify 4/4 DONE (VERIFICATION.json), deterministic self-play 89,186 twice, worst call 1.16 s, overage left >= 59.7 s,
glibc <= 2.27 (same as the live hyb-melon68 bundle). Kaggle kernel check (BC, finished right after the submit): PASSED - kernel
pavelsavchenkov/kag-d3m68-m3-check on the same archive (sha f8c82bd8), Kaggle image glibc 2.35, env 1.32.7: 4/4 DONE, rewards equal local
(89,186 / 271,547 / 249,977), rebuilt bridge same rewards, worst call 2.67 s, overage left >= 46.9 s (kaggle_check/output/). Keys-off
identity: the same bridge without the new keys reproduces the v62 bundle exactly (87,602 / 240,904 / 250,505).
Archive sha256 f8c82bd813f7ab3e63adef8383a7990fd902f40f292045672d14e5105cf5ac17.
Retires hyb-melon68 (56682211, 2805) from the two active submissions; the other active one is giovanni-rl-v1023-d3crop (56688520).
