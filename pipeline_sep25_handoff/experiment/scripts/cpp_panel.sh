#!/bin/sh
# The 7 C++ opponents (seeds 700-715, both seats) for one candidate: a panel of varied opponents for
# changes that the mirror cannot judge (e.g. the opponent forecast).
# usage: scripts/cpp_panel.sh <cand dir> <build> <threads> <out dir>
cd "$(dirname "$0")/.."
cand=$1; build=$2; threads=$3; out=$4
mkdir -p $out
for o in agent_sep23 king_rc4 teammate_shoprouter arlene_v4_m31 ahmed_v25 investment_context_guarded_001_best two_random_shop_league_v179; do
    [ -f $out/cpp_$o.csv ] || BC_OPUS_MODEL=$cand/model.bin $build/full_games $o 700 16 $threads $out/cpp_$o.csv >> $out/cpp.log 2>&1
done
