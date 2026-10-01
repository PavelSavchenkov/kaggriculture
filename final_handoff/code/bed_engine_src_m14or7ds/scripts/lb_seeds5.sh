#!/bin/sh
# Like scripts/lb_seeds3.sh for the Local-LB active set after pavel-bc-opus-v12-wages (e2229aa, expected: cha22 out;
# challenger excluded): opponent i (alphabetical, from 1) plays seeds 1000*i .. +19, both seats.
# ONLY=<agent>: that opponent alone.
# usage: scripts/lb_seeds5.sh <out_dir> [procs]   (env: BUILD, BC_OPUS_MODEL)
cd "$(dirname "$0")/.."
out=$1; procs=${2:-8}
i=0
for a in ahmed-productive-wheat-v54 arsgorynich-herd-safe-v3 pavel-bc-opus-v12-herd pavel-bc-opus-v12-robust \
         pavel-bc-opus-v12-slots pavel-bc-opus-v12-vadim pavel-bc-opus-v12-wages shiiin9-order-book \
         ttyn-kaggriculture-master-engine-v3 wzhengbiao-v15stack; do
    i=$((i + 1)); first=$((1000 * i))
    [ -n "$ONLY" ] && [ "$ONLY" != "$a" ] && continue
    LB_SNAPSHOT=localLB_main conda run -n kaggriculture python scripts/lb_play.py $out $a --seeds $first-$((first + 19)) \
        --procs $procs >> $out.log 2>&1
done
