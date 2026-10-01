#!/bin/sh
# Screening mirror: candidate folders vs a reference candidate, one seat per seed (the maps are mirrored
# and both agents are deterministic, so the second seat mostly repeats the game: 1267 of 2360 seat pairs
# were identical). Seeds <seed_start>..+19, seat 0. Output reports/mirror1/<name>_<seed_start>.{csv,log}.
# usage: scripts/mirror1.sh <reference cand dir> <build> <threads> <seed_start> <cand dir> [...]
cd "$(dirname "$0")/.."
ref=$1; build=$2; threads=$3; start=$4; shift 4
mkdir -p reports/mirror1
for c in "$@"; do
    name=$(basename $c)
    log=reports/mirror1/${name}_$start.log
    [ -s $log ] && continue
    SEARCH_SEAT=0 BC_OPUS_MODEL=$c/model.bin $build/search_games bc:$ref/model.bin $start 20 $threads reports/mirror1/${name}_$start.csv 0 > $log 2>&1
done
