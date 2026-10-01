#!/bin/bash
# Like-for-like LB projection: each run dir restricted to the (opponent, seed, seat) games present in ALL given run dirs, then the LB's own
# Bradley-Terry fit (lbproject.py) per arm. usage: proj_common.sh <label> <tag> ...   -> out/projc_<label>/<tag>/ (symlinks) + table
cd "$(dirname "$0")"; lab=$1; shift; S=/home/pavel/Programming/kaggriculture/experiments/v10/sep24_BC_opus/data/localLB_d6157df
common=$(for t in "$@"; do ls out/$t/ | grep json; done | sort | uniq -c | awk -v n=$# '$1 == n {print $2}')
echo "$lab: $(echo "$common" | wc -l) common games across $*"
for t in "$@"; do d=out/projc_$lab/$t; mkdir -p $d; for f in $common; do ln -sf $(readlink -f out/$t/$f) $d/$f; done
  conda run --no-capture-output -n kaggriculture python lbproject.py $S $t $d 2>&1 | grep -v -i warn | grep -E "challenger games|<- challenger|dc12m19-fc2-m68$" ; done
