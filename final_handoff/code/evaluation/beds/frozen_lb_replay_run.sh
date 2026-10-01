#!/bin/bash
# Frozen-replay sanity bed: Local-LB games (Day compiler's exact C++ traces of #1's official games vs m14-fc2 / mmpq-v2); our
# candidate in #1's seat, the sibling's recorded actions replayed frozen (DUEL_OPP_REC, loans), build_m14or6 (m19 keys).
# usage: [PRIO=lo] run.sh <tag> <model dir> [list, default list_all.txt]
X0=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_mm_copy
tag=$1; model=$(readlink -f "$2")/model.bin; L=${3:-list_all.txt}; BIN=$X0/build_m14or6/duel_mm; out=$tag
[ -f "$model" ] && [ -x "$BIN" ] || { echo "lbfrozen $tag: missing"; exit 1; }
cd $X0/runs/lbfrozen; mkdir -p $out
$X0/scripts/manifest_guard.sh $out/MANIFEST.txt $BIN $(dirname $model) || { echo "lbfrozen $tag refused (manifest)"; exit 3; }
cat $L | while read tr seat sub; do
  id=$(basename $(dirname $tr))_$(basename $tr .txt); n=$out/$id; [ -s $n.csv ] && continue
  echo "$tr $seat $sub" > $n.lst
  echo "cd $PWD && RUNQ_SESSION=Imitation /home/pavel/Programming/kaggriculture/work/runq/slot.sh -p ${PRIO:-lo} -t lbfrozen:$tag env REPLAY_SHOPS=1 DUEL_OPP_REC=1 DUEL_SELL=$n.sell DUEL_SELL_ALL=1 DUEL_DAYS=$n.days BC_OPUS_MODEL=$model nice -n 10 $BIN $n.lst 1 $n.csv > /dev/null 2> $n.err"
done | xargs -d '\n' -n 1 -P 6 sh -c
echo "lbfrozen $tag done: $(ls $out | grep -c csv)"
