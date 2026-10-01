#!/bin/bash
# Forecaster redesign, first models (Sep 30, BC). Data: build_train.py (Kaggle only; recent Sep 27-30 games as team "recent").
# fc1 = the deployed big2_stk_xs recipe with --shift 0 (the rotation was misaligned) and the new population; fc2 = fc1 + --xlag 1
# (yesterday's hourly sales of all 7 products: cross-product input the C++ runtime already supports, FCT4). One GPU job at a time.
set -e
F=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_bc_mm/fc; W=/home/pavel/Programming/kaggriculture/work/sep26_wide_losses
D=$F/d1
cd $W
py() { OMP_NUM_THREADS=4 nice -n 5 conda run -n kaggriculture --no-capture-output python "$@"; }
py $F/build_train.py $D
py fc_tf.py prepare $D
common="--shift 0 --weight-old 0.5 --stockin 1 --xseen 1 --val val_recent --weight-teams $D/recent.csv:2"
py fc_tf_intra.py train $D fc1 $common 2> $D/fc1.log | tee -a $D/runs.jsonl
py fc_tf_intra.py train $D fc2 $common --xlag 1 2> $D/fc2.log | tee -a $D/runs.jsonl
echo train_fc done
