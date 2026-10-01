#!/bin/bash
# G3 with a chosen live opponent list (LIST in this dir), build_m14or6 (m19; no opponent replay), full games, head N.
# usage: [BIN=<duel_mm>] LIST=list_field_early.txt runb.sh <tag> <model dir> <N>
X0=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_mm_copy
tag=$1; model=$(readlink -f "$2")/model.bin; N=$3; BIN=${BIN:-$X0/build_m14or6/duel_mm}; out=$tag; L=$X0/runs/fieldopp/$LIST
[ -f "$model" ] && [ -x "$BIN" ] && [ -f "$L" ] || { echo "fieldopp $tag: missing model / bin / list"; exit 1; }
cd $X0/runs/fieldopp; mkdir -p $out
$X0/scripts/manifest_guard.sh $out/MANIFEST.txt $BIN $(dirname $model) || { echo "fieldopp $tag refused (manifest)"; exit 3; }
head -$N $L | while read tr seat sub; do
  id=$(basename $tr .txt); n=$out/$id; [ -s $n.csv ] && continue
  echo "$tr $seat $sub" > $n.lst
  echo "cd $PWD && RUNQ_SESSION=Imitation /home/pavel/Programming/kaggriculture/work/runq/slot.sh -p hi -t fieldopp:$tag env REPLAY_SHOPS=1 DUEL_SELL=$n.sell DUEL_SELL_ALL=1 DUEL_DAYS=$n.days DUEL_HERD=$n.herd BC_OPUS_MODEL=$model nice -n 10 $BIN $n.lst 1 $n.csv > /dev/null 2> $n.err"
done | xargs -d '\n' -n 1 -P 6 sh -c
echo "fieldopp $tag done: $(ls $out | grep -c csv) / $N"
