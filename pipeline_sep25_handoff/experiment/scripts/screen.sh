#!/bin/sh
# Screening panel (cheaper than panel.sh) for one model (BC_OPUS_MODEL) and optional env:
#   reports/screen/<name>/lb     Local-LB behaviour groups (LB_OPPS), seeds SEEDS
#   reports/screen/<name>/cpp_*  C++ opponents (CPP_OPPS), seeds SEEDS (both seats)
# Defaults: 4 Local-LB groups, 4 C++ opponents, seeds 700-715 (128 + 128 games).
# BUILD selects the build directory (default build). Compare with scripts/panel_compare.py.
# usage: scripts/screen.sh <name> [procs]
set -e
cd "$(dirname "$0")/.."
name=$1; procs=${2:-26}
out=reports/screen/$name
seeds=${SEEDS:-700-715}
first=${seeds%-*}; last=${seeds#*-}
mkdir -p $out
conda run -n kaggriculture python scripts/lb_play.py $out/lb \
    ${LB_OPPS:-arsgorynich-herd-safe-v3 ahmed-productive-wheat-v54 arlene-farmer-john-idle-seller yannik2-replay-champion} \
    --seeds $seeds --procs $procs > $out/lb.log 2>&1
for o in ${CPP_OPPS:-king_rc4 ahmed_v25 arlene_v4_m31 teammate_shoprouter}; do
    [ -f $out/cpp_$o.csv ] || ${BUILD:-build}/full_games $o $first $((last - first + 1)) $procs $out/cpp_$o.csv >> $out/cpp.log 2>&1
done
echo done
