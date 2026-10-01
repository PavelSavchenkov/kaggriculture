#!/bin/sh
# On-policy value-model data: full games of the model under test (BC_OPUS_MODEL, its sidecar
# compiler settings) with random single-day decision pushes on 25% of days 0-12 (BC_PERTURB),
# against C++ opponents, BC clones and itself; every game saved as a trace.
# usage: scripts/value_data.sh <out_dir> <seed_start> <games_per_opponent> [threads]
cd "$(dirname "$0")/.."
out=$1; start=$2; games=$3; threads=${4:-8}
for o in king_rc4 ahmed_v25 arlene_v4_m31 teammate_shoprouter agent_sep23 bc:models/zoo_majkel/model.bin \
         bc:models/zoo_dsm/model.bin bc:models/cand_so2/model.bin; do
    name=$(echo $o | sed 's|bc:models/||; s|/model.bin||')
    [ -f $out/$name.csv ] && continue
    mkdir -p $out/$name
    BC_PERTURB=${BC_PERTURB:-0.25} BC_WRITE_TRACES=$out/$name build_vd/full_games $o $start $games $threads $out/$name.csv >> $out/log.txt 2>&1
done
