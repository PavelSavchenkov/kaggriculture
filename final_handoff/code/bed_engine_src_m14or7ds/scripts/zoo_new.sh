#!/bin/sh
# New-meta BC clones: v12 fine-tuned on one team's games with its Sep 24+ games x5 (arrays_meta4 / arrays_meta2mg,
# strength 150 / day index 41), 3000 steps, lr 1e-4, the team's style all game, sidecars of zoo_<team>_race.
# A reactive bed of the current Kaggle meta (the zoo_<team>_race clones learned Sep 18-23 play).
# usage: scripts/zoo_new.sh   -> models/zoo_<team>_new
cd "$(dirname "$0")/.."
for pair in boey:16915014:meta4 mm:16681125:meta4 vadim:16770421:meta4 decem:16623559:meta4 goose:16730612:meta2mg majkel:16718819:meta2mg; do
    name=${pair%%:*}; rest=${pair#*:}; team=${rest%%:*}; new=${rest#*:}
    out=models/zoo_${name}_new
    [ -s $out/model.bin ] && continue
    while ps -eo args | grep -q '^python -u scripts/train.py\|^python scripts/train.py'; do sleep 20; done
    mkdir -p $out
    m=$(for i in 1 2 3 4 5; do printf ",data/arrays_$new"; done)
    conda run --no-capture-output -n kaggriculture python -u scripts/train.py --arrays data/arrays_v5$m --out $out \
        --init models/v12_cond/model.pt --teams $team --lr 1e-4 --steps 3000 --width 256 --grid --marginal --style \
        --condition data/conditions_v5.csv,data/conditions_${new}_s150.csv --gpu-data > $out/train.log 2>&1
    for s in compiler decode features; do cp models/zoo_${name}_race/model.bin.$s $out/; done
    cp models/zoo_${name}_race/model.bin.style $out/
done
# Sep 27: three more current teams from corpus_fresh_sep26 (Fourth Quadrant plays 4 quadrants: max_land 4).
for pair in fq:16667959:15 azat:16743356:11 fish:16854176:10; do
    name=${pair%%:*}; rest=${pair#*:}; team=${rest%%:*}; style=${rest#*:}
    out=models/zoo_${name}_new
    [ -s $out/model.bin ] && continue
    while ps -eo args | grep -q '^python -u scripts/train.py\|^python scripts/train.py'; do sleep 20; done
    mkdir -p $out
    m=$(for i in 1 2 3 4 5; do printf ",data/arrays_fresh_sep26"; done)
    conda run --no-capture-output -n kaggriculture python -u scripts/train.py --arrays data/arrays_v5$m --out $out \
        --init models/v12_cond/model.pt --teams $team --lr 1e-4 --steps 3000 --width 256 --grid --marginal --style \
        --condition data/conditions_v5.csv,data/conditions_fresh_sep26_s150.csv --gpu-data > $out/train.log 2>&1
    for s in compiler features; do cp models/zoo_dsm_race/model.bin.$s $out/; done
    sed "s/^max_land 3/max_land $([ $name = fq ] && echo 4 || echo 3)/" models/zoo_dsm_race/model.bin.decode > $out/model.bin.decode
    echo $style > $out/model.bin.style
done
