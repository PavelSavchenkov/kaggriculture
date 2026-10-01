#!/bin/bash
# Sequential stop for a Stage-2 run: every 60 s reads the paired games with seqtest.py; at the first REJECT / PROMOTE (checkpoints
# 24 / 48 / 96 / 192 / max) it writes runq/stop/<tag>, so the tag's queued slot.sh jobs exit without running, and logs the verdict in
# runq/verdicts.log. Run it in the background next to the launcher.
# usage: seqwatch.sh <tag> <max games> <seqtest.py args: --a '<candidate glob>' --b '<baseline glob>' [--key ..] [--delta ..]>
R=/home/pavel/Programming/kaggriculture/work/runq
tag=$1; max=$2; shift 2
while true; do
  out=$(conda run -n kaggriculture --no-capture-output python $R/seqtest.py --max $max "$@" 2>&1)
  if echo "$out" | grep -q "REJECT\|PROMOTE\|max reached"; then
    touch "$R/stop/${tag//\//_}"
    echo "$(date '+%m-%d %H:%M') $tag | $(echo "$out" | tr '\n' ' ')" >> $R/verdicts.log
    echo "$out"; exit 0
  fi
  sleep 60
done
