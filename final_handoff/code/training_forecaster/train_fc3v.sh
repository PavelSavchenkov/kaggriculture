#!/bin/bash
# fc3v / fc3nv (astra 14:30): fc3 / fc3n retrained with a clean checkpoint-selection set (d3: val_recent minus recent_ours and bed
# episodes). fc3v = bed worlds out of TRAIN; fc3nv = + recent_ours rows and our team's 2 old rows out of TRAIN. Same seed and recipe.
set -e
F=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_bc_mm/fc; W=/home/pavel/Programming/kaggriculture/work/sep26_wide_losses
D=$F/d3
cd $W
py() { OMP_NUM_THREADS=4 nice -n 5 conda run -n kaggriculture --no-capture-output python "$@"; }
py $F/build_train3.py $F/d2 $D
py fc_tf.py prepare $D
common="--shift 0 --weight-old 0.5 --stockin 1 --xseen 1 --val val_recent --weight-teams $D/recent.csv:2 --xlag 1"
py fc_tf_intra.py train $D fc3v $common 2> $D/fc3v.log | tee -a $D/runs.jsonl
py fc_tf_intra.py train $D fc3nv $common --exclude recent_ours --exclude-teams $D/ours.csv 2> $D/fc3nv.log | tee -a $D/runs.jsonl
echo train_fc3v done
