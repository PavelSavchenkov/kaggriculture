#!/bin/sh
# BC zoo: fine-tunes of a base model on single top teams (league opponents and candidates).
# Each model gets the team's style index as its default style (<model>.style).
# usage: scripts/zoo.sh base_model_dir
cd "$(dirname "$0")/.."
base=$1
for pair in dsm:16732748 decem:16623559 goose:16730612 majkel:16718819 vadim:16770421 mm:16681125; do
    name=${pair%%:*}; team=${pair#*:}
    out=models/zoo_${name}
    [ -f $out/model.bin ] && continue
    mkdir -p $out
    conda run --no-capture-output -n kaggriculture python -u scripts/train.py --arrays data/arrays_v5 --out $out \
        --init $base/model.pt --teams $team --lr 1e-4 --steps 3000 --width 256 --grid --marginal --style \
        --condition data/conditions_v5.csv > $out/train.log 2>&1
    conda run -n kaggriculture python -c "import json; print(json.load(open('data/styles.json'))['styles'].get('$team', 0))" > $out/model.bin.style
done
