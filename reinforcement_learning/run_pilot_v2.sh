#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LB_ROOT="${KAGGRICULTURE_LB_ROOT:-$(dirname "${REPO_ROOT}")/kaggriculture-localLB}"
OUTPUT="${REPO_ROOT}/reinforcement_learning/artifacts/seeds-pilot-v3"
PREFIX="rl-pilot3"
RESUME=0
for argument in "$@"; do
  if [[ "${argument}" == "--resume" ]]; then
    RESUME=1
  fi
done

cd "${REPO_ROOT}"
echo "[pilot 1/6] build and test the engine/model interfaces"
conda run --no-capture-output -n kaggle cmake -S reinforcement_learning \
  -B reinforcement_learning/build -G Ninja
conda run --no-capture-output -n kaggle cmake --build reinforcement_learning/build
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python -m pytest -q reinforcement_learning/tests

if [[ -e "${OUTPUT}/global.pt" && "${RESUME}" -eq 0 ]]; then
  echo "Refusing to overwrite ${OUTPUT}; rerun this script with --resume." >&2
  exit 2
fi

echo "[pilot 2/6] train four full-resolution recurrent lineage seeds"
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/train_seed_models.py \
    --replays corpus/data_all \
    --output "${OUTPUT}" \
    --max-seeds 4 \
    --require-seeds 4 \
    --min-trajectories 40 \
    --min-win-rate 0.52 \
    --max-trajectories-per-seed 128 \
    --validation-fraction 0.15 \
    --stride 1 \
    --burn-in 168 \
    --sequence-length 48 \
    --batch-size 128 \
    --device cuda \
    --global-steps 5000 \
    --finetune-steps 2000 \
    --learning-rate 0.0001 \
    --finetune-learning-rate 0.00005 \
    --warmup-updates 250 \
    --checkpoint-every 250 \
    --validation-batches 50 \
    --validation-every 500 \
    --log-every 25 \
    "$@"

echo "[pilot 3/6] export the checkpoints as exact LocalLB submission directories"
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/export_seed_agents.py \
    --input "${OUTPUT}" \
    --output-root "${LB_ROOT}/agents" \
    --agent-prefix "${PREFIX}" \
    --overwrite

mapfile -t AGENTS < <(find "${LB_ROOT}/agents" -maxdepth 1 -type d \
  -name "${PREFIX}-*" -printf '%f\n' | sort)
if [[ "${#AGENTS[@]}" -ne 4 ]]; then
  echo "Expected four exported agents, found ${#AGENTS[@]}." >&2
  exit 3
fi

echo "[pilot 4/6] validate the exact exported directories with LocalLB"
VALIDATED=0
for agent_name in "${AGENTS[@]}"; do
  VALIDATED=$((VALIDATED + 1))
  echo "[validate agents] ${VALIDATED}/${#AGENTS[@]} ${agent_name}"
  PYTHONPATH="${LB_ROOT}/src" conda run --no-capture-output -n kaggle python \
    -m lb.cli validate --config "${LB_ROOT}/config/leaderboard.yaml" \
    --agent-dir "${LB_ROOT}/agents/${agent_name}"
done

echo "[pilot 5/6] play full 720-step paired games, including seat swaps"
python arena/arena.py \
  --lb-root "${LB_ROOT}" \
  --agents baseline-random "${AGENTS[@]}" \
  --seeds-per-pair 2 \
  --seed-base 41000 \
  --episode-steps 720 \
  --procs 8 \
  --cores-per-worker 1 \
  --out arena/out/rl_pilot_v3.json

echo "[pilot 6/6] require every seed to beat the always-PASS baseline"
python reinforcement_learning/tools/check_arena_gate.py \
  arena/out/rl_pilot_v3.json \
  --prefix "${PREFIX}" \
  --baseline baseline-random

echo "[pilot complete] training=${OUTPUT}/training_report.json"
echo "[pilot complete] exports=${LB_ROOT}/agents/seed_export_report.json"
echo "[pilot complete] closed_loop=${REPO_ROOT}/arena/out/rl_pilot_v3.json"
