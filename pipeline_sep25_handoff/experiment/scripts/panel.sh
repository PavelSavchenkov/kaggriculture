#!/bin/sh
# Evaluation panel for one model (BC_OPUS_MODEL=<model.bin>, default: models/selected):
#   reports/panel/<name>/lb       one Local-LB agent per behaviour group, seeds 700-731
#       (identical games: shiiin9 = arsgorynich; arlene-wheat = ahmed; haideptry = sunil = arlene-idle)
#   reports/panel/<name>/extra    diverse public-notebook and teammate agents, seeds 700-707
#   reports/panel/<name>/cpp_*    C++ opponents, seeds 700-715 (both seats)
#   reports/panel/<name>/replay.csv  frozen top-team replays (data/replay_opponents.txt)
# BUILD selects the build directory (default build).
# usage: scripts/panel.sh <name> [procs]
set -e
cd "$(dirname "$0")/.."
name=$1; procs=${2:-26}
out=reports/panel/$name
mkdir -p $out
py() { conda run -n kaggriculture python "$@"; }
py scripts/lb_play.py $out/lb arsgorynich-herd-safe-v3 ahmed-productive-wheat-v54 arlene-farmer-john-idle-seller \
    cha22-route-replay yannik2-replay-champion latest-yannik-suffix-5d-28s-v1 --seeds 700-731 --procs $procs > $out/lb.log 2>&1
py scripts/lb_play.py $out/extra extra:candidate_frontier_moon_v189c extra:candidate_frontier_soil_v202a \
    extra:candidate_kaito_v54_exact extra:candidate_moon_sheep4 extra:candidate_v24_meta_tree_safe extra:rule_forge_v1 extra:team_ppo_v6n_it040 extra:team_rl_v6_02_it1520 \
    --seeds 700-707 --procs $procs > $out/extra.log 2>&1
for o in agent_sep23 king_rc4 teammate_shoprouter arlene_v4_m31 ahmed_v25 investment_context_guarded_001_best \
         two_random_shop_league_v179; do
    [ -f $out/cpp_$o.csv ] || ${BUILD:-build}/full_games $o 700 16 $procs $out/cpp_$o.csv >> $out/cpp.log 2>&1
done
[ -f $out/replay.csv ] || ${BUILD:-build}/replay_games data/replay_opponents.txt $procs $out/replay.csv > $out/replay.log 2>&1
echo done
