#!/bin/sh
# Large C++ panel for statistical power (the model and its settings come from the
# environment, as in screen.sh): 4 C++ opponents x seeds 1000-1127 x both seats
# (1,024 games) into reports/bigcpp/<name>/cpp_*.csv. Compare with panel_compare.py.
# usage: scripts/bigcpp.sh <name> [procs]
cd "$(dirname "$0")/.."
name=$1; procs=${2:-10}
out=reports/bigcpp/$name
mkdir -p $out
for o in ahmed_v25 king_rc4 arlene_v4_m31 teammate_shoprouter; do
    [ -f $out/cpp_$o.csv ] || ${BUILD:-build}/full_games $o 1000 128 $procs $out/cpp_$o.csv >> $out/cpp.log 2>&1
done
