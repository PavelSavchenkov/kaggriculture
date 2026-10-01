#!/bin/bash
# Challenger folder = a snapshot agent folder (BASE, default the m19-fc2 folder: main.py, bridge, AGENT.toml, source) with model/ replaced by
# <model dir> (cp -rL). usage: [BASE=<snapshot agent id>] mkagent.sh <name> <model dir>
cd "$(dirname "$0")"; n=$1; m=$(readlink -f $2); d=agents/$n; A=/home/pavel/Programming/kaggriculture/experiments/v10/sep24_BC_opus/data/localLB_d6157df/agents/${BASE:-pavel-bc-opus-v17d-dc12m19-fc2-m68}
[ -d $d ] && { echo "exists: $d"; exit 0; }
[ -d $A ] || { echo "stop: no base $A"; exit 1; }
mkdir -p $d; for f in $(ls $A | grep -v "^model$\|^__pycache__$"); do cp -rL $A/$f $d/; done; cp -rL $m $d/model; chmod -R u+w $d
[ -z "$(find $d -type l)" ] || { echo "stop: symlinks in $d"; exit 1; }
echo "base $A" > $d/SRC_MODEL.txt; echo "$m" >> $d/SRC_MODEL.txt; (cd $m && find -L . -type f | sort | while read f; do echo "$f $(sha256sum "$f" | cut -c1-16)"; done) >> $d/SRC_MODEL.txt
echo "made $d"
