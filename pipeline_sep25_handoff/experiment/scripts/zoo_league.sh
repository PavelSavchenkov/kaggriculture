#!/bin/sh
# Network-quality gate: the model under test (BC_OPUS_MODEL, with its sidecars) against BC clones
# of top Kaggle teams (models/zoo_*), C++ full games, seeds 1300-1315 both seats; both sides use
# the same compiler build, so only the networks differ.
# usage: scripts/zoo_league.sh <out_dir> [procs]   (env: BUILD, BC_OPUS_MODEL)
cd "$(dirname "$0")/.."
out=$1; procs=${2:-8}
mkdir -p $out
for zoo in zoo_dsm zoo_majkel zoo_vadim zoo_decem zoo_goose zoo_mm; do
    [ -f $out/$zoo.csv ] || ${BUILD:-build}/full_games bc:models/$zoo/model.bin 1300 16 $procs $out/$zoo.csv >> $out/league.log 2>&1
done
