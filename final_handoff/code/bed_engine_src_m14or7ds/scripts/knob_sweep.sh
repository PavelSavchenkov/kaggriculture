#!/bin/sh
# Decision-push screening: each config plays arsgorynich and ahmed on seeds 700-715
# (both seats) into reports/knobs/<name>; compare with scripts/lb_compare.py against
# the unpushed games of the same seeds.
cd "$(dirname "$0")/.."
run() {
    name=$1; shift
    [ -d reports/knobs/$name ] && return
    env "$@" conda run -n kaggriculture python scripts/lb_play.py reports/knobs/$name \
        arsgorynich-herd-safe-v3 ahmed-productive-wheat-v54 --seeds 700-715 --procs 26 > reports/knobs/$name.log 2>&1
}
mkdir -p reports/knobs
run q_crop_035 BC_Q_CROP=0.35
run q_crop_065 BC_Q_CROP=0.65
run q_animal_035 BC_Q_ANIMAL=0.35
run q_animal_065 BC_Q_ANIMAL=0.65
run land_plus2 BC_LAND_BIAS=2
run land_minus2 BC_LAND_BIAS=-2
run care_all BC_FEED_ALL=1 BC_CARE_ALL=1
run collect_all BC_COLLECT_ALL=1
run harvest_all BC_HARVEST_ALL=1
