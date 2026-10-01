#!/bin/bash
# hybrid-opponent bed (build_m14or7): the opponent replays the real team's recorded farm (loans) and its own agent's seller (the list's
# opponent model) sells products 1-7; wheat / fertilizer sales stay recorded. Arm: our model, or M&M's recording (ARMREC=1).
# usage: [ARMREC=1] run.sh <tag> <model dir> <N>
X0=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_mm_copy; S=/home/pavel/Programming/kaggriculture/work/sep29_validation/swap
tag=$1; model=$(readlink -f "$2")/model.bin; N=$3; BIN=$X0/build_m14or7/duel_mm; out=$tag
[ -f "$model" ] && [ -x "$BIN" ] || { echo "hybopp $tag: missing"; exit 1; }
A=""; [ -n "$ARMREC" ] && A="DUEL_REC= DUEL_LOAN=1 DUEL_REC_UNTIL=${UNTIL:-30}"
cd $X0/runs/hybopp; mkdir -p $out
$X0/scripts/manifest_guard.sh $out/MANIFEST.txt $BIN $(dirname $model) || { echo "hybopp $tag refused (manifest)"; exit 3; }
head -$N ${LIST:-$S/list_g3w218_clean_mix.txt} | while read tr seat sub; do
  id=$(basename $tr .txt); n=$out/$id; [ -s $n.csv ] && continue
  echo "$tr $seat $sub" > $n.lst
  echo "cd $PWD && RUNQ_SESSION=Imitation /home/pavel/Programming/kaggriculture/work/runq/slot.sh -p ${PRIO:-hi} -t hybopp:$tag env REPLAY_SHOPS=1 $A DUEL_OPP_REC=1 DUEL_OPP_SELL=1,2,3,4,5,6,7 DUEL_SELL=$n.sell DUEL_SELL_ALL=1 DUEL_DAYS=$n.days BC_OPUS_MODEL=$model nice -n 10 $BIN $n.lst 1 $n.csv > /dev/null 2> $n.err"
done | xargs -d '\n' -n 1 -P 6 sh -c
echo "hybopp $tag done: $(ls $out | grep -c csv) / $N"
