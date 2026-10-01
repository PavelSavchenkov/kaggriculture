#!/bin/sh
# Broad unseen-seed panel: all 10 active Local-LB agents (data/localLB_main = Local-LB main),
# both seats. Knobs and model come from the environment (BUILD, BC_OPUS_MODEL, DC10_*, BC_*).
# usage: scripts/panel_lb10.sh <out_dir> <seeds a-b> [procs]
cd "$(dirname "$0")/.."
out=$1; seeds=$2; procs=${3:-10}
mkdir -p $out
LB_SNAPSHOT=localLB_main conda run -n kaggriculture python scripts/lb_play.py $out/lb ahmed-productive-wheat-v54 \
    arlene-farmer-john-idle-seller arlene-farmer-john-wheat-seller arsgorynich-herd-safe-v3 cha22-route-replay \
    haideptry-master-hybrid-2965 shiiin9-order-book sunil-idle-workers wzhengbiao-v15stack yannik2-replay-champion \
    --seeds $seeds --procs $procs > $out/lb.log 2>&1
