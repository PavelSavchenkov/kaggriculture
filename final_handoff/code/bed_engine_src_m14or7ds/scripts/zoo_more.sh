#!/bin/sh
# More agent-zoo clones (same recipe as scripts/zoo.sh: v12 fine-tuned 3000 steps on one team's games,
# the team's style as default), plus <name>_race variants with our race compiler and decoding
# (fin_Z_rs_l3 sidecars) for the zoo gate. Waits for any running train.py first (one training at a time).
# usage: scripts/zoo_more.sh base_model_dir [name:team_id ...]   (default: the five Sep 26 teams)
cd "$(dirname "$0")/.."
base=$1; shift
pairs=${*:-boey:16915014 mtmr:16758882 kaggledew:16633132 thirdfarm:16730524 fish:16854176}
while ps -eo cmd | grep -v grep | grep -q "scripts/train.py"; do sleep 30; done
for pair in $pairs; do
    name=${pair%%:*}; team=${pair#*:}
    out=models/zoo_${name}
    if [ ! -f $out/model.bin ]; then
        mkdir -p $out
        conda run --no-capture-output -n kaggriculture python -u scripts/train.py --arrays data/arrays_v5 --out $out \
            --init $base/model.pt --teams $team --lr 1e-4 --steps 3000 --width 256 --grid --marginal --style \
            --condition data/conditions_v5.csv --gpu-data > $out/train.log 2>&1
        conda run -n kaggriculture python -c "import json; print(json.load(open('data/styles.json'))['styles'].get('$team', 0))" > $out/model.bin.style
    fi
    race=models/zoo_${name}_race
    mkdir -p $race
    cp $out/model.bin $out/model.bin.condition $out/model.bin.features $out/model.bin.style $race/
    cp models/fin_Z_rs_l3/model.bin.compiler models/fin_Z_rs_l3/model.bin.decode $race/
done
