#!/bin/bash
# Exact Local-LB games through the team gate: every game = runq/slot.sh + lbgame.py. Resumes (skips existing JSONs).
# usage: [RUNQ_PRIO=hi] [P=12] lbrun.sh <tag> <challenger agent dir> <opponent:first_seed:n_seeds> ...   -> out/<tag>/<opp>_<seed>_seat<k>.json
cd "$(dirname "$0")"
tag=$1; chal=$(readlink -f $2); shift 2; P=${P:-12}; S=/home/pavel/Programming/kaggriculture/experiments/v10/sep24_BC_opus/data/localLB_d6157df
mkdir -p out/$tag; echo "$(date '+%F %T') challenger $chal snapshot $S specs $*" >> out/$tag/lists.txt
for spec in "$@"; do opp=${spec%%:*}; r=${spec#*:}; first=${r%%:*}; n=${r#*:}
  for ((s = first; s < first + n; s++)); do for seat in 0 1; do
    f=out/$tag/${opp}_${s}_seat$seat.json; [ -s $f ] && continue
    echo "cd $PWD && RUNQ_SESSION=Weaknesses /home/pavel/Programming/kaggriculture/work/runq/slot.sh ${RUNQ_PRIO:+-p $RUNQ_PRIO} -t lb:$tag env OMP_NUM_THREADS=1 nice -n 5 conda run --no-capture-output -n kaggriculture python lbgame.py $S $chal $opp $s $seat $f"
  done; done
done | xargs -P $P -I{} sh -c "{} > /dev/null 2>&1"
echo "$tag done: $(ls out/$tag/*.json 2>/dev/null | wc -l) games"
