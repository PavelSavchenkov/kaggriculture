#!/bin/sh
# Flexibility test: which part of the agent moves outcomes against the strongest Local-LB
# agents? Network pushes (decoding knobs, opening style) vs compiler variants, each played on
# the same fresh games (seeds 1200-1207, both seats, arsgorynich / ahmed / wzhengbiao; snapshot
# data/localLB_febb5a8). Output: reports/flex/<setting>/lb. Compare with scripts/flex_report.py.
# usage: scripts/flex.sh [procs]
cd "$(dirname "$0")/.."
procs=${1:-14}
for setting in base: \
    n_crop35:BC_Q_CROP=0.35 n_crop65:BC_Q_CROP=0.65 n_animal35:BC_Q_ANIMAL=0.35 n_animal65:BC_Q_ANIMAL=0.65 \
    n_land_up:BC_LAND_BIAS=2 n_land_down:BC_LAND_BIAS=-2 n_sample:BC_SAMPLE=1 n_open_dsm:BC_OPENING_STYLE=7 \
    c_trim_crops:DC10_TRIM_CROPS_FIRST=1 c_trim_model:DC10_TRIM_MODEL=1 c_ladder_fast:DC10_LADDER_EFFORT=fast \
    c_hold_spot:DC10_HOLD_SPOT=1 c_rival_half:DC10_RIVAL_WEIGHT=0.5 c_rival_stock20:DC10_RIVAL_STOCK_FROM=20 \
    c_no_reserve:DC10_NO_RESERVE=1 c_no_cash_guard:DC10_NO_CASH_GUARD=1; do
    name=${setting%%:*}; knob=${setting#*:}
    out=reports/flex/$name
    [ -d $out/lb ] && [ $(ls $out/lb | wc -l) -ge 48 ] && continue
    mkdir -p $out
    env $knob LB_SNAPSHOT=localLB_febb5a8 BUILD=${BUILD:-build_ab4} BC_OPUS_MODEL=models/cand_v12_vadim6/model.bin \
        conda run -n kaggriculture python scripts/lb_play.py $out/lb arsgorynich-herd-safe-v3 ahmed-productive-wheat-v54 \
        wzhengbiao-v15stack --seeds 1200-1207 --procs $procs > $out/lb.log 2>&1
done
