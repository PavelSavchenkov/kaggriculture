#!/bin/sh
# Exact Local-LB games of a challenger under the current rules (20 seeds per pair, both seats; each pair's seeds from
# the snapshot's own round-robin matrix: work/sep25_bc_opus_snippets/lb_new_pairs.py), strongest opponents first.
# usage: scripts/lb_exact.sh <pairs file: opponent first last> <out dir> [procs]  (env: LB_SNAPSHOT, BUILD, BC_OPUS_MODEL)
cd "$(dirname "$0")/.."
pairs=$1; out=$2; procs=${3:-4}
{ grep "pavel-bc-opus" $pairs; grep -v "pavel-bc-opus" $pairs; } | while read a first last; do
    conda run -n kaggriculture python scripts/lb_play.py $out $a --seeds $first-$last --procs $procs >> $out.log 2>&1
done
