#!/bin/sh
# Expert-iteration data: search-improved games (tools/search_games, days 0-SEARCH_LAST_DAY,
# 7 day-level pushes) played by BC_OPUS_MODEL against real opponents while rollouts use a
# DIFFERENT opponent model (labels must not exploit one opponent). Traces of both seats go
# to reports/expert_iter/<real>__<model>/traces; our seat is the label source.
# OUT: output root (default reports/expert_iter); BUILD: build dir with search_games.
# usage: scripts/expert_iter.sh seed_start games threads
cd "$(dirname "$0")/.."
start=$1; games=$2; threads=$3
for pair in ahmed_v25:king_rc4 king_rc4:ahmed_v25 arlene_v4_m31:teammate_shoprouter teammate_shoprouter:arlene_v4_m31 \
            bc:models/zoo_majkel/model.bin:bc:models/zoo_vadim/model.bin bc:models/zoo_vadim/model.bin:bc:models/zoo_majkel/model.bin; do
    case $pair in
        bc:*) real=$(echo $pair | cut -d: -f1-2); model=$(echo $pair | cut -d: -f3-4) ;;
        *) real=${pair%%:*}; model=${pair#*:} ;;
    esac
    name=$(echo "${real}__${model}" | sed 's|bc:models/||g; s|/model.bin||g; s|[:/]|_|g')
    out=${OUT:-reports/expert_iter}/$name
    [ -f $out/games.csv ] && continue
    mkdir -p $out/traces
    SEARCH_LAST_DAY=${SEARCH_LAST_DAY:-6} SEARCH_MODEL=$model SEARCH_TRACES=$out/traces \
        ${BUILD:-build_snap2}/search_games $real $start $games $threads $out/games.csv 1 > $out/log.txt 2>&1
done
