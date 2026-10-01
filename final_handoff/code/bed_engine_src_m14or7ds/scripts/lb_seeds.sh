#!/bin/sh
# Replays the Local-LB's own evaluation games locally: opponent i (alphabetical active list)
# plays seeds 1000 + 1000*i .. +19, both seats (src/lb/match.py _pair_seeds, pair_index_base 0).
# usage: scripts/lb_seeds.sh <out_dir> [procs]   (env: BUILD, BC_OPUS_MODEL, DC10_*/BC_* knobs)
cd "$(dirname "$0")/.."
out=$1; procs=${2:-8}
i=0
for a in ahmed-productive-wheat-v54 arlene-farmer-john-idle-seller arlene-farmer-john-wheat-seller arsgorynich-herd-safe-v3 \
         cha22-route-replay haideptry-master-hybrid-2965 shiiin9-order-book sunil-idle-workers wzhengbiao-v15stack \
         yannik2-replay-champion; do
    i=$((i + 1)); first=$((1000 * i))
    LB_SNAPSHOT=localLB_main conda run -n kaggriculture python scripts/lb_play.py $out $a --seeds $first-$((first + 19)) \
        --procs $procs >> $out.log 2>&1
done
