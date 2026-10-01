#!/bin/sh
# Network screen under one compiler: each listed network (weights, condition, features) gets the
# reference candidate's opening and compiler sidecars and plays C++ mirror games against the
# reference (seeds 1300-1319, both seats).
# usage: scripts/net_mirror.sh <reference cand dir> <build> <threads> <model dir> [...]
cd "$(dirname "$0")/.."
ref=$1; build=$2; threads=$3; shift 3
mkdir -p reports/netmirror
for m in "$@"; do
    name=$(basename $m)
    out=models/netm_$name
    mkdir -p $out
    cp $m/model.bin $m/model.bin.condition $m/model.bin.features $out/
    cp $ref/model.bin.opening $ref/model.bin.compiler $out/
    [ -s reports/netmirror/$name.log ] && continue
    BC_OPUS_MODEL=$out/model.bin $build/search_games bc:$ref/model.bin 1300 20 $threads reports/netmirror/$name.csv 0 \
        > reports/netmirror/$name.log 2>&1
done
