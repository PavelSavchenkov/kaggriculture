#!/bin/bash
# Compile-time optimisation check: replays the recorded candidate games
# (reports/lb_dump_cand) with an lb_replay binary, compares every day's report and the
# hash of every emitted action with a baseline label, and prints compile totals.
# Games run in parallel, one per physical P-core (taskset), to limit timing noise.
# usage: scripts/opt_check.sh <label> [lb_replay binary, default build_dev/lb_replay] [baseline label, default ref]
cd "$(dirname "$0")/.."
label=$1; bin=${2:-build_dev/lb_replay}; base=${3:-ref}
core=${CORE0:-0}
for f in reports/lb_dump_cand/*.bin; do
    n=$(basename $f .bin)
    BC_OPUS_MODEL=${MODEL:-models/cand_v12_vadim6/model.bin} taskset -c $core $bin $f > reports/opt/${label}_$n.txt 2>/dev/null &
    core=$(( (core + 2) % ${CORES:-16} ))
done
wait
same=0; all=0
for f in reports/lb_dump_cand/*.bin; do
    n=$(basename $f .bin)
    all=$((all + 1))
    cmp -s <(sed 's/ ms [0-9]*//' reports/opt/${label}_$n.txt) <(sed 's/ ms [0-9]*//' reports/opt/${base}_$n.txt) && same=$((same + 1))
done
echo "$label vs $base: games with identical reports and actions $same / $all"
conda run -n kaggriculture python -c "
import re,glob
t=[];m=[]
for f in glob.glob('reports/opt/${label}_*.txt'):
    ms=[int(re.search(r' ms (\d+)',l).group(1)) for l in open(f) if ' ms ' in l]; t.append(sum(ms)/1000); m.append(max(ms))
print('$label: mean compile %.2f s per game, max game %.2f s, worst day %d ms'%(sum(t)/len(t), max(t), max(m)))"
