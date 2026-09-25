#!/bin/sh
# Inference-setting screen: variants of the reference candidate (same weights and compiler
# sidecar) with another opening ("days style") and/or strength condition ("known strength/200
# day/40"), in C++ mirror games against the reference.
# SEEDS: seeds per variant (default 20, both seats each).
# usage: scripts/knob_mirror.sh <reference cand dir> <build> <threads> <seed_start> name:opening:condition [...]
#        (empty opening/condition: keep the reference's)
cd "$(dirname "$0")/.."
ref=$1; build=$2; threads=$3; start=$4; shift 4
mkdir -p reports/knobmirror
for v in "$@"; do
    name=${v%%:*}; rest=${v#*:}; opening=${rest%%:*}; condition=${rest#*:}
    out=models/knob_$name
    mkdir -p $out
    cp $ref/model.bin $ref/model.bin.features $ref/model.bin.compiler $ref/model.bin.condition $ref/model.bin.opening $out/
    [ -n "$opening" ] && echo "$opening" > $out/model.bin.opening
    [ -n "$condition" ] && echo "$condition" > $out/model.bin.condition
    log=reports/knobmirror/${name}_$start.log
    [ -s $log ] && continue
    BC_OPUS_MODEL=$out/model.bin $build/search_games bc:$ref/model.bin $start ${SEEDS:-20} $threads reports/knobmirror/${name}_$start.csv 0 > $log 2>&1
done
