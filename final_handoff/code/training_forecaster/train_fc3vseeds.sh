#!/bin/bash
# fc3v seed ensemble (Sep 30, BC; Imitation: clean members for swap-type beds): fc3v recipe and data (d3) with seeds 1 and 2. The C++ runtime averages <model>.forecast_tf,
# .forecast_tf.2, .forecast_tf.3 (dawn and intra-day heads), so an ensemble deploys without code changes. Offline check: fc_avg.py.
set -e
F=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_bc_mm/fc; W=/home/pavel/Programming/kaggriculture/work/sep26_wide_losses
D=$F/d3
cd $W
py() { OMP_NUM_THREADS=4 nice -n 5 conda run -n kaggriculture --no-capture-output python "$@"; }
common="--shift 0 --weight-old 0.5 --stockin 1 --xseen 1 --val val_recent --weight-teams $D/recent.csv:2 --xlag 1"
for s in 1 2; do py fc_tf_intra.py train $D fc3vs$s $common --seed $s 2> $D/fc3vs$s.log | tee -a $D/runs.jsonl; done
for s in 1 2; do
  py fc_tf_export.py $D/fc3vs$s.pt $D $F/export/fc3vs$s.bin 2>&1 | tail -1
  probe/build/fc_tf_check $F/export/fc3vs$s.bin 2>&1 | tail -1
  py $F/fc_eval.py $D/fc3vs$s.pt $F/eval_recent $F/eval_recent/eval_fc3vs$s.csv
  py $F/fc_eval.py $D/fc3vs$s.pt $F/eval_weak $F/eval_weak/eval_fc3vs$s.csv
done
cd $F/eval_recent && py $F/fc_avg.py eval_fc3vens3.csv eval_fc3v.csv eval_fc3vs1.csv eval_fc3vs2.csv && py $F/fc_clean.py eval_fc3v.csv eval_fc3vs1.csv eval_fc3vs2.csv eval_fc3vens3.csv eval_fc2ens3.csv > clean800_fc3vens.txt
cd $F/eval_weak && py $F/fc_avg.py eval_fc3vens3.csv eval_fc3v.csv eval_fc3vs1.csv eval_fc3vs2.csv && py $F/fc_groups.py eval_fc3v.csv eval_fc3vs1.csv eval_fc3vs2.csv eval_fc3vens3.csv eval_fc2ens3.csv > weak_fc3vens.txt
echo fc3vseeds done
