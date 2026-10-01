#!/bin/bash
# opponent-replay bed (build_m14or, DUEL_OPP_REC=1): our model in M&M's seat vs M&M's real opponent's recorded play (with loans), G3 clean
# mixed list head N. usage: run.sh <tag> <model dir> <N>
X0=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_mm_copy; S=/home/pavel/Programming/kaggriculture/work/sep29_validation/swap
tag=$1; model=$(readlink -f "$2")/model.bin; N=$3; BIN=$X0/build_m14or6/duel_mm; out=$tag
[ -f "$model" ] && [ -x "$BIN" ] || { echo "opprec $tag: no $model or $BIN"; exit 1; }
cd $X0/runs/opprec; mkdir -p $out
$X0/scripts/manifest_guard.sh $out/MANIFEST.txt $BIN $(dirname $model) || { echo "opprec $tag refused (manifest)"; exit 3; }
head -$N ${LIST:-$S/list_g3w218_clean_mix.txt} | while read tr seat sub; do
  id=$(basename $tr .txt); n=$out/$id; [ -s $n.csv ] && continue
  echo "$tr $seat $sub" > $n.lst
  echo "cd $PWD && RUNQ_SESSION=Imitation /home/pavel/Programming/kaggriculture/work/runq/slot.sh -p ${PRIO:-hi} -t opprec:$tag env REPLAY_SHOPS=1 DUEL_OPP_REC=1 DUEL_SELL=$n.sell DUEL_SELL_ALL=1 DUEL_DAYS=$n.days BC_OPUS_MODEL=$model nice -n 10 $BIN $n.lst 1 $n.csv > /dev/null 2> $n.err"
done | xargs -d '\n' -n 1 -P 6 sh -c
echo "opprec $tag done: $(ls $out | grep -c csv) / $N"
