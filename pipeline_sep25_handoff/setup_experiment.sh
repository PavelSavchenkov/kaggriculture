#!/bin/sh
# Creates a working experiment folder from this handoff, laid out like the original
# experiments/v10/sep24_BC_opus (the Python scripts find the repository 3 levels up):
#   <dest>/                 copy of experiment/ (sources, tools, scripts, small data)
#   <dest>/models/v12_cond  the network (model.bin, model.pt, sidecars, training log)
#   <dest>/models/<name>    one folder per candidate in candidates/ (model.bin hard-linked)
# Build, bulk data and Local-LB snapshot: see EVALUATION.md and TRAINING.md.
# usage: pipeline_sep25_handoff/setup_experiment.sh experiments/<version>/<date>_<name>
set -e
handoff=$(cd "$(dirname "$0")" && pwd)
dest=$1
[ -n "$dest" ] && [ ! -e "$dest" ] || { echo "usage: $0 <new experiment dir>"; exit 1; }
mkdir -p "$(dirname "$dest")"
cp -r "$handoff/experiment" "$dest"
mkdir -p "$dest/models" "$dest/reports"
cp -r "$handoff/weights/v12_cond" "$dest/models/v12_cond"
for candidate in "$handoff"/candidates/*/; do
    name=$(basename "$candidate")
    mkdir -p "$dest/models/$name"
    cp "$candidate"/* "$dest/models/$name/"
    ln "$dest/models/v12_cond/model.bin" "$dest/models/$name/model.bin"
done
echo "created $dest; models: $(ls "$dest/models" | tr '\n' ' ')"
