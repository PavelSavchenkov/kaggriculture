#!/bin/sh
# League of our own BC versions (C++ full games, seeds 700-715, both seats): the model
# under test (BC_OPUS_MODEL) against each listed model. Results join the panel as
# reports/panel/<name>/cpp_bc_<label>.csv.
# usage: scripts/league.sh <name> [procs] -- label=model.bin ...
cd "$(dirname "$0")/.."
name=$1; procs=${2:-26}; shift 3
out=reports/panel/$name
mkdir -p $out
for pair in "$@"; do
    label=${pair%%=*}; model=${pair#*=}
    [ -f $out/cpp_bc_$label.csv ] || build/full_games bc:$model 700 16 $procs $out/cpp_bc_$label.csv >> $out/league.log 2>&1
done
