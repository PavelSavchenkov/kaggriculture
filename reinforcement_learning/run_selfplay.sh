#!/usr/bin/env bash
# Train one independent PPO descendant from every promoted behavior clone.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LB_ROOT="${KAGGRICULTURE_LB_ROOT:-$(dirname "${REPO_ROOT}")/kaggriculture-localLB}"
PROMOTION_MANIFEST="${PROMOTION_MANIFEST:-${REPO_ROOT}/reinforcement_learning/artifacts/seeds-pilot-elo-v2/promoted_agents.json}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${REPO_ROOT}/reinforcement_learning/artifacts/selfplay-v1}"
UPDATES="${UPDATES:-100}"
ENVIRONMENTS="${ENVIRONMENTS:-32}"
EPISODE_STEPS="${EPISODE_STEPS:-720}"
SEQUENCE_LENGTH="${SEQUENCE_LENGTH:-24}"
SEQUENCES_PER_MINIBATCH="${SEQUENCES_PER_MINIBATCH:-32}"
PPO_EPOCHS="${PPO_EPOCHS:-1}"
EVALUATION_EVERY="${EVALUATION_EVERY:-5}"
EVALUATION_SEEDS="${EVALUATION_SEEDS:-2}"
RECORD_KAGZ_EVERY="${RECORD_KAGZ_EVERY:-1}"
SNAPSHOT_EVERY="${SNAPSHOT_EVERY:-5}"
MAX_SNAPSHOTS="${MAX_SNAPSHOTS:-8}"
ACTOR_LEARNING_RATE="${ACTOR_LEARNING_RATE:-0.000001}"
CRITIC_LEARNING_RATE="${CRITIC_LEARNING_RATE:-0.0001}"
TARGET_KL="${TARGET_KL:-0.01}"
DEVICE="${DEVICE:-cuda}"
SKIP_PREFLIGHT="${SKIP_PREFLIGHT:-0}"
PREFLIGHT_ONLY="${PREFLIGHT_ONLY:-0}"
RUN_FINAL_EVALUATION="${RUN_FINAL_EVALUATION:-1}"

require_positive_int() {
  local name="$1"
  local value="$2"
  if [[ ! "$value" =~ ^[1-9][0-9]*$ ]]; then
    echo "$name must be a positive integer; got '$value'" >&2
    exit 2
  fi
}

require_nonnegative_int() {
  local name="$1"
  local value="$2"
  if [[ ! "$value" =~ ^[0-9]+$ ]]; then
    echo "$name must be a non-negative integer; got '$value'" >&2
    exit 2
  fi
}

require_positive_int UPDATES "$UPDATES"
require_positive_int ENVIRONMENTS "$ENVIRONMENTS"
require_positive_int EPISODE_STEPS "$EPISODE_STEPS"
require_positive_int SEQUENCE_LENGTH "$SEQUENCE_LENGTH"
require_positive_int SEQUENCES_PER_MINIBATCH "$SEQUENCES_PER_MINIBATCH"
require_positive_int PPO_EPOCHS "$PPO_EPOCHS"
require_positive_int EVALUATION_EVERY "$EVALUATION_EVERY"
require_positive_int EVALUATION_SEEDS "$EVALUATION_SEEDS"
require_positive_int SNAPSHOT_EVERY "$SNAPSHOT_EVERY"
require_positive_int MAX_SNAPSHOTS "$MAX_SNAPSHOTS"
require_nonnegative_int RECORD_KAGZ_EVERY "$RECORD_KAGZ_EVERY"

if [[ "$EPISODE_STEPS" -gt 720 ]]; then
  echo "EPISODE_STEPS must be at most 720; got '$EPISODE_STEPS'" >&2
  exit 2
fi
if [[ "$DEVICE" != "cuda" && "$DEVICE" != "cpu" ]]; then
  echo "DEVICE must be cuda or cpu; got '$DEVICE'" >&2
  exit 2
fi

if [[ ! -f "${PROMOTION_MANIFEST}" ]]; then
  echo "Promotion manifest not found: ${PROMOTION_MANIFEST}" >&2
  exit 2
fi
mapfile -t LEARNERS < <(jq -r '.accepted[]' "${PROMOTION_MANIFEST}")
if [[ "${#LEARNERS[@]}" -eq 0 ]]; then
  echo "Promotion manifest has no accepted agents." >&2
  exit 3
fi

if [[ ! -d "${LB_ROOT}/agents" ]]; then
  echo "LocalLB agents directory not found: ${LB_ROOT}/agents" >&2
  exit 2
fi
for learner in "${LEARNERS[@]}"; do
  if [[ ! -f "${LB_ROOT}/agents/${learner}/export_manifest.json" ]]; then
    echo "Accepted agent is missing its LocalLB export manifest: ${LB_ROOT}/agents/${learner}/export_manifest.json" >&2
    exit 2
  fi
done

cd "${REPO_ROOT}"

if [[ "$SKIP_PREFLIGHT" != "1" ]]; then
  echo "[self-play preflight 1/3] configure and build the exact game engine"
  conda run --no-capture-output -n kaggle \
    cmake -S reinforcement_learning -B reinforcement_learning/build -G Ninja
  conda run --no-capture-output -n kaggle \
    cmake --build reinforcement_learning/build

  echo "[self-play preflight 2/3] run reinforcement-learning regression tests"
  PYTHONPATH=reinforcement_learning/python \
    conda run --no-capture-output -n kaggle python -m pytest -q \
      reinforcement_learning/tests

  echo "[self-play preflight 3/3] validate the requested compute device"
  SELFPLAY_DEVICE="$DEVICE" conda run --no-capture-output -n kaggle python -c \
    'import os, torch; d=os.environ["SELFPLAY_DEVICE"]; assert d == "cpu" or torch.cuda.is_available(), "CUDA requested but PyTorch cannot see a GPU"; print("device=cpu" if d == "cpu" else f"device={d} gpu={torch.cuda.get_device_name(0)} vram_gib={torch.cuda.get_device_properties(0).total_memory / 2**30:.1f}")'
else
  echo "[self-play preflight] skipped because SKIP_PREFLIGHT=1"
fi

echo "Self-play population: ${#LEARNERS[@]} independently trained descendants"
echo "Promotion manifest: ${PROMOTION_MANIFEST}"
echo "Output root: ${OUTPUT_ROOT}"
echo "Settings: updates=${UPDATES} envs=${ENVIRONMENTS} episode_steps=${EPISODE_STEPS} sequence_length=${SEQUENCE_LENGTH} minibatch_sequences=${SEQUENCES_PER_MINIBATCH} ppo_epochs=${PPO_EPOCHS} device=${DEVICE}"
echo "Safety: actor_lr=${ACTOR_LEARNING_RATE} critic_lr=${CRITIC_LEARNING_RATE} target_kl=${TARGET_KL} eval_every=${EVALUATION_EVERY} eval_seeds=${EVALUATION_SEEDS}"

if [[ "$PREFLIGHT_ONLY" == "1" ]]; then
  echo "[self-play] preflight complete; PREFLIGHT_ONLY=1, so no training was started"
  exit 0
fi

echo "[self-play population] 0/${#LEARNERS[@]} starting"
for index in "${!LEARNERS[@]}"; do
  learner="${LEARNERS[$index]}"
  destination="${OUTPUT_ROOT}/${learner}"
  echo "[self-play population] $((index + 1))/${#LEARNERS[@]} learner=${learner}"
  PYTHONPATH=reinforcement_learning/python \
    conda run --no-capture-output -n kaggle python \
      reinforcement_learning/tools/train_selfplay.py \
      --promotion-manifest "${PROMOTION_MANIFEST}" \
      --lb-root "${LB_ROOT}" \
      --learner-agent "${learner}" \
      --output "${destination}" \
      --updates "${UPDATES}" \
      --environments "${ENVIRONMENTS}" \
      --episode-steps "${EPISODE_STEPS}" \
      --sequence-length "${SEQUENCE_LENGTH}" \
      --sequences-per-minibatch "${SEQUENCES_PER_MINIBATCH}" \
      --ppo-epochs "${PPO_EPOCHS}" \
      --evaluation-every "${EVALUATION_EVERY}" \
      --evaluation-seeds "${EVALUATION_SEEDS}" \
      --record-kagz-every "${RECORD_KAGZ_EVERY}" \
      --snapshot-every "${SNAPSHOT_EVERY}" \
      --max-snapshots "${MAX_SNAPSHOTS}" \
      --actor-learning-rate "${ACTOR_LEARNING_RATE}" \
      --critic-learning-rate "${CRITIC_LEARNING_RATE}" \
      --target-kl "${TARGET_KL}" \
      --device "${DEVICE}" "$@"
done
echo "[self-play population complete] ${OUTPUT_ROOT}"

if [[ "$RUN_FINAL_EVALUATION" == "1" ]]; then
  echo "[self-play final evaluation] export, validate, and run fresh LocalLB games"
  SELFPLAY_ROOT="$OUTPUT_ROOT" \
  PROMOTION_MANIFEST="$PROMOTION_MANIFEST" \
  KAGGRICULTURE_LB_ROOT="$LB_ROOT" \
    "$REPO_ROOT/reinforcement_learning/evaluate_selfplay.sh"
else
  echo "[self-play final evaluation] skipped because RUN_FINAL_EVALUATION=$RUN_FINAL_EVALUATION"
fi
