#!/bin/sh
# C++ mirror games of existing candidate folders against a reference candidate (both use their own
# sidecars), seeds <seed_start>..+19, both seats. Output reports/candmirror/<name>_<seed_start>.
# usage: scripts/cand_mirror.sh <reference cand dir> <build> <threads> <seed_start> <cand dir> [...]
cd "$(dirname "$0")/.."
ref=$1; build=$2; threads=$3; start=$4; shift 4
mkdir -p reports/candmirror
for c in "$@"; do
    name=$(basename $c)
    log=reports/candmirror/${name}_$start.log
    [ -s $log ] && continue
    BC_OPUS_MODEL=$c/model.bin $build/search_games bc:$ref/model.bin $start 20 $threads reports/candmirror/${name}_$start.csv 0 > $log 2>&1
done
