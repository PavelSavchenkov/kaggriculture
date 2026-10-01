#!/bin/sh
# Expert iteration with exact copies (our Local-LB opponents are our own lineage): search_games plays
# BC_OPUS_MODEL (with its sidecars) against an exact copy of each opponent, searching the 7 decision
# pushes at every dawn up to SEARCH_LAST_DAY (default 12) with full-game rollouts; every game is saved
# as a trace (our seat is the label source for scripts/build_search_corpus.py).
# usage: scripts/expert_iter2.sh <out_root> <seed_start> <games> <threads> <opponent model> [...]
cd "$(dirname "$0")/.."
out=$1; start=$2; games=$3; threads=$4; shift 4
for m in "$@"; do
    name=$(basename $(dirname $m))
    dir=$out/$name
    [ -f $dir/games.csv ] && continue
    mkdir -p $dir/traces
    SEARCH_LAST_DAY=${SEARCH_LAST_DAY:-12} SEARCH_TRACES=$dir/traces build_ei/search_games bc:$m $start $games $threads \
        $dir/games.csv 1 > $dir/log.txt 2>&1
done
