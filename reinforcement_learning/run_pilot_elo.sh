#!/usr/bin/env bash
# Strength-targeted BC pilot: exact Kaggle submission identity and rating.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LB_ROOT="${KAGGRICULTURE_LB_ROOT:-$(dirname "${REPO_ROOT}")/kaggriculture-localLB}"
OUTPUT="${OUTPUT:-${REPO_ROOT}/reinforcement_learning/artifacts/seeds-pilot-elo-v2}"
PREFIX="${PREFIX:-rl-elo2}"
RATINGS="${RATINGS:-corpus/index/episode_agents.csv}"
REPLAYS="${REPLAYS:-corpus/data_all}"
SEEDS="${SEEDS:-4}"
SUBMISSION_IDS="${SUBMISSION_IDS:-56065395,56064371,56064002,56078126}"
MIN_ELO="${MIN_ELO:-2600}"
MIN_TRAJECTORIES="${MIN_TRAJECTORIES:-40}"
MIN_MARGIN="${MIN_MARGIN:-}"
MIN_PROMOTED="${MIN_PROMOTED:-3}"
BATCH_SIZE="${BATCH_SIZE:-32}"
GLOBAL_STEPS="${GLOBAL_STEPS:-5000}"
FINETUNE_STEPS="${FINETUNE_STEPS:-2000}"
LEARNING_RATE="${LEARNING_RATE:-0.0001}"
CLUSTER_MANIFEST="${OUTPUT}/selected_lineages.json"

RESUME=0
for argument in "$@"; do
  if [[ "${argument}" == "--resume" ]]; then RESUME=1; fi
done

cd "${REPO_ROOT}"
if [[ ! -f "${RATINGS}" ]]; then
  echo "Submission-level ratings file ${RATINGS} not found." >&2
  echo "Create it with: conda run -n kaggle python corpus/kaggle_index.py --seed <submission-id> --max-calls 400 --out corpus/index" >&2
  exit 2
fi

echo "[pilot 1/7] build and test engine/model interfaces"
conda run --no-capture-output -n kaggle cmake -S reinforcement_learning \
  -B reinforcement_learning/build -G Ninja
conda run --no-capture-output -n kaggle cmake --build reinforcement_learning/build
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python -m pytest -q reinforcement_learning/tests

if [[ -e "${OUTPUT}/global.pt" && "${RESUME}" -eq 0 ]]; then
  echo "Refusing to overwrite ${OUTPUT}; rerun this script with --resume." >&2
  exit 2
fi
mkdir -p "${OUTPUT}"

echo "[pilot 2/7] select exact submissions by latest Kaggle rating"
SELECT_ARGS=(--ratings "${RATINGS}" --replays "${REPLAYS}"
             --max-seeds "${SEEDS}" --min-elo "${MIN_ELO}"
             --min-trajectories "${MIN_TRAJECTORIES}"
             --manifest "${CLUSTER_MANIFEST}"
             --out "${OUTPUT}/lineage_selection.json")
IFS=',' read -r -a REQUESTED_IDS <<< "${SUBMISSION_IDS}"
if [[ "${#REQUESTED_IDS[@]}" -ne "${SEEDS}" ]]; then
  echo "SUBMISSION_IDS must contain exactly ${SEEDS} comma-separated IDs." >&2
  exit 3
fi
for submission_id in "${REQUESTED_IDS[@]}"; do
  SELECT_ARGS+=(--submission-id "${submission_id}")
done
if [[ -n "${MIN_MARGIN}" ]]; then SELECT_ARGS+=(--min-margin "${MIN_MARGIN}"); fi
conda run --no-capture-output -n kaggle python \
  reinforcement_learning/tools/select_lineages.py "${SELECT_ARGS[@]}"

echo "[pilot 3/7] train ${SEEDS} recurrent submission lineages"
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/train_seed_models.py \
    --replays "${REPLAYS}" --output "${OUTPUT}" \
    --cluster-manifest "${CLUSTER_MANIFEST}" \
    --max-seeds "${SEEDS}" --require-seeds "${SEEDS}" \
    --min-trajectories "${MIN_TRAJECTORIES}" --max-trajectories-per-seed 128 \
    --validation-fraction 0.15 --stride 1 --burn-in 168 --sequence-length 48 \
    --batch-size "${BATCH_SIZE}" --device cuda \
    --global-steps "${GLOBAL_STEPS}" --finetune-steps "${FINETUNE_STEPS}" \
    --learning-rate "${LEARNING_RATE}" --finetune-learning-rate 0.00005 \
    --warmup-updates 250 --checkpoint-every 250 \
    --validation-batches 50 --validation-every 500 --log-every 25 "$@"

echo "[pilot 4/7] export exact LocalLB submission directories"
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/export_seed_agents.py \
    --input "${OUTPUT}" --output-root "${LB_ROOT}/agents" \
    --agent-prefix "${PREFIX}" --overwrite

mapfile -t AGENTS < <(find "${LB_ROOT}/agents" -maxdepth 1 -type d \
  -name "${PREFIX}-*" -printf '%f\n' | sort)
if [[ "${#AGENTS[@]}" -ne "${SEEDS}" ]]; then
  echo "Expected ${SEEDS} exported agents, found ${#AGENTS[@]}." >&2
  exit 4
fi

echo "[pilot 5/7] validate exported directories with LocalLB"
for index in "${!AGENTS[@]}"; do
  agent_name="${AGENTS[$index]}"
  echo "[validate agents] $((index + 1))/${#AGENTS[@]} ${agent_name}"
  PYTHONPATH="${LB_ROOT}/src" conda run --no-capture-output -n kaggle python \
    -m lb.cli validate --config "${LB_ROOT}/config/leaderboard.yaml" \
    --agent-dir "${LB_ROOT}/agents/${agent_name}"
done

echo "[pilot 6/7] play full 720-step paired games with seat swaps"
python arena/arena.py --lb-root "${LB_ROOT}" \
  --agents baseline-random "${AGENTS[@]}" --seeds-per-pair 2 \
  --seed-base 41000 --episode-steps 720 --procs 8 --cores-per-worker 1 \
  --out arena/out/rl_pilot_elo_v2.json

echo "[pilot 7/7] qualify seeds independently against the always-PASS baseline"
python reinforcement_learning/tools/check_arena_gate.py \
  arena/out/rl_pilot_elo_v2.json --prefix "${PREFIX}" --baseline baseline-random \
  --minimum-promoted "${MIN_PROMOTED}" \
  --promotion-manifest "${OUTPUT}/promoted_agents.json"

echo "[pilot complete] selection=${OUTPUT}/lineage_selection.json"
echo "[pilot complete] training=${OUTPUT}/training_report.json"
echo "[pilot complete] exports=${LB_ROOT}/agents/seed_export_report.json"
echo "[pilot complete] closed_loop=${REPO_ROOT}/arena/out/rl_pilot_elo_v2.json"
echo "[pilot complete] promoted=${OUTPUT}/promoted_agents.json"
