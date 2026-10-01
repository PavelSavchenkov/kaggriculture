#!/bin/sh
# Old gates beyond the Local-LB (as scripts/panel.sh): extra Python agents (seeds 700-707),
# C++ opponents (seeds 700-715, both seats) and frozen top-team replays.
# usage: scripts/gates.sh <out_dir> [procs]   (env: BUILD, BC_OPUS_MODEL, knobs)
cd "$(dirname "$0")/.."
out=$1; procs=${2:-8}
mkdir -p $out
conda run -n kaggriculture python scripts/lb_play.py $out/extra extra:candidate_frontier_moon_v189c extra:candidate_frontier_soil_v202a \
    extra:candidate_kaito_v54_exact extra:candidate_moon_sheep4 extra:candidate_v24_meta_tree_safe extra:rule_forge_v1 \
    extra:team_ppo_v6n_it040 extra:team_rl_v6_02_it1520 --seeds 700-707 --procs $procs > $out/extra.log 2>&1
for o in agent_sep23 king_rc4 teammate_shoprouter arlene_v4_m31 ahmed_v25 investment_context_guarded_001_best \
         two_random_shop_league_v179; do
    [ -f $out/cpp_$o.csv ] || ${BUILD:-build}/full_games $o 700 16 $procs $out/cpp_$o.csv >> $out/cpp.log 2>&1
done
[ -f $out/replay.csv ] || REPLAY_SHOPS=1 REPLAY_WEEDS=1 ${BUILD:-build}/replay_games data/replay_opponents.txt $procs $out/replay.csv > $out/replay.log 2>&1
