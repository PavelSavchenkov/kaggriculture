#!/bin/sh
# Expert-iteration fine-tune: our seat of the search-improved games (scripts/expert_iter2.sh traces)
# as extra BC labels. Builds the corpus, extracts arrays (4 shards on E-cores) and fine-tunes v12 from
# its weights at a low learning rate on arrays_v5 plus the search arrays repeated REPEAT times; search
# rows use the DSM opening style for days 0-5 (as the agent played them).
# usage: scripts/ei2_finetune.sh <out model dir> <steps> <trace dir> [...]   (env: REPEAT=10, LR=0.0001)
set -e
cd "$(dirname "$0")/.."
out=$1; steps=$2; shift 2
data=data/ei2
rm -rf $data && mkdir -p $data/arrays
conda run -n kaggriculture python scripts/build_search_corpus.py $data/corpus.txt $data/conditions.csv "$@"
for k in 0 1 2 3; do
    taskset -c $((16 + 2 * k))-$((17 + 2 * k)) build_dev/extract $data/corpus.txt $k 4 $data/arrays/shard_0$k > $data/arrays/shard_0$k.err 2>&1 &
done
wait
arrays=data/arrays_v5
for i in $(seq ${REPEAT:-10}); do arrays=$arrays,$data/arrays; done
mkdir -p $out
conda run --no-capture-output -n kaggriculture python scripts/train.py --arrays $arrays --out $out \
    --init models/v12_cond/model.pt --steps $steps --batch 512 --lr ${LR:-0.0001} --width 256 --seed 0 --grid --marginal \
    --style --style-dropout 0.1 --search-opening "6 7" --condition data/conditions_v5.csv,$data/conditions.csv \
    --condition-dropout 0.1 --gpu-data > $out/train.log 2>&1
