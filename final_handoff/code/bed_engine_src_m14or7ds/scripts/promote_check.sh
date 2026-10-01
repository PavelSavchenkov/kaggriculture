#!/bin/sh
# Promotion gates for a candidate model folder, in priority order (cheapest decisive test first):
#   1. agent zoo: top-team BC clones using our race compiler (models/zoo_<team>_race), seeds 1300 and
#      1400 (16 each, one seat): reports/promote/<cand>/zoo_race_<set>/<team>.csv
#   2. agent zoo, clones with their default compiler (models/zoo_<team>), seeds 1300-1315
#   3. head to head vs the reference (unmerged Local-LB #1 candidate), one seat, seed sets 1300 1400
#      1500 7000
#   4. static top-team replays (data/replay_opponents.txt, 145 games, opponent frozen)
#   5. exact Local-LB games vs the active roster (ROSTER file, default reports/roster_top30_eb1a039.txt;
#      SEEDS_PER_PAIR, default 7) through the Local-LB bridge
# Stops after a gate with STOP_AFTER=<n>. Threads per run: THREADS (default 4). Clones: ZOO_TEAMS (default the
# original six: dsm decem goose majkel mm vadim; added Sep 26: boey mtmr kaggledew thirdfarm fish).
# SHOP_CRN=1 (tools/shop_crn.hpp: shops fixed per seed and index, paired games stay paired) writes to
# reports/promote_crn/ instead of reports/promote/. Zoo seed sets: ZOO_SETS (default "1300 1400").
# Clones: models/zoo_<team>${CLONE_SUFFIX:-_race} (_raceff: clones racing like top players).
# Binaries: BUILD (default build_x), FULL_TOOL (default full_games; full_games_dc11: our seat plays dc11). Output root: OUT_ROOT (default reports/promote or reports/promote_crn).
# usage: scripts/promote_check.sh <candidate model dir> <reference model dir>
set -e
cd "$(dirname "$0")/.."
cand=$1; ref=$2; t=${THREADS:-4}; stop=${STOP_AFTER:-5}; b=${BUILD:-build_x}; fg=${FULL_TOOL:-full_games}
teams=${ZOO_TEAMS:-dsm decem goose majkel mm vadim}
name=$(basename $cand)
out=${OUT_ROOT:-reports/promote${SHOP_CRN:+_crn}}/$name
mkdir -p $out
for set in ${ZOO_SETS:-1300 1400}; do
    mkdir -p $out/zoo_race_$set
    for z in $teams; do
        [ -s $out/zoo_race_$set/$z.csv ] || FULL_SEAT=0 BC_OPUS_MODEL=$cand/model.bin nice -n 5 $b/$fg \
            bc:models/zoo_${z}${CLONE_SUFFIX:-_race}/model.bin $set 16 $t $out/zoo_race_$set/$z.csv > /dev/null 2>&1
    done
done
[ $stop -le 1 ] && exit 0
mkdir -p $out/zoo_default
for z in $teams; do
    [ -s $out/zoo_default/$z.csv ] || FULL_SEAT=0 BC_OPUS_MODEL=$cand/model.bin nice -n 5 $b/$fg \
        bc:models/zoo_$z/model.bin 1300 16 $t $out/zoo_default/$z.csv > /dev/null 2>&1
done
[ $stop -le 2 ] && exit 0
for set in 1300 1400 1500 7000; do
    [ -s $out/h2h_$set.csv ] || SEARCH_SEAT=0 BC_OPUS_MODEL=$cand/model.bin nice -n 5 $b/search_games \
        bc:$ref/model.bin $set 20 $t $out/h2h_$set.csv 0 > /dev/null 2>&1
done
[ $stop -le 3 ] && exit 0
[ -s $out/replay.csv ] || REPLAY_SHOPS=1 REPLAY_WEEDS=1 BC_OPUS_MODEL=$cand/model.bin nice -n 5 $b/replay_games data/replay_opponents.txt $t \
    $out/replay.csv > /dev/null 2>&1
[ $stop -le 4 ] && exit 0
BUILD=$b BC_OPUS_MODEL=$cand/model.bin SEEDS_PER_PAIR=${SEEDS_PER_PAIR:-7} nice -n 5 scripts/lb_seeds_roster.sh \
    ${ROSTER:-reports/roster_top30_eb1a039.txt} $out/lb $t
