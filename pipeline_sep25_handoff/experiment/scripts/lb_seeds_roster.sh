#!/bin/sh
# Exact Local-LB games of a challenger under the Sep 26 rules (src/lb/match.py _pair_seeds): the
# k-th active agent in alphabetical order (from 0, challenger excluded) plays seeds
# 1000 * (k + 1) .. + SEEDS_PER_PAIR - 1, both seats. Roster: one agent id per line (active agents of
# data/rankings.json after the rebuild). ONLY=<agent>: that opponent alone.
# usage: scripts/lb_seeds_roster.sh <roster.txt> <out_dir> [procs]   (env: BUILD, BC_OPUS_MODEL, SEEDS_PER_PAIR=7)
cd "$(dirname "$0")/.."
roster=$1; out=$2; procs=${3:-8}; n=${SEEDS_PER_PAIR:-7}
k=0
for a in $(sort "$roster"); do
    first=$((1000 * (k + 1))); k=$((k + 1))
    [ -n "$ONLY" ] && [ "$ONLY" != "$a" ] && continue
    LB_SNAPSHOT=localLB_main conda run -n kaggriculture python scripts/lb_play.py $out $a --seeds $first-$((first + n - 1)) \
        --procs $procs >> $out.log 2>&1
done
