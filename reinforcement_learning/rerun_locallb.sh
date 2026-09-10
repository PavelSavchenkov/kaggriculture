#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LB_ROOT="${KAGGRICULTURE_LB_ROOT:-$(dirname "${REPO_ROOT}")/kaggriculture-localLB}"
SEEDS_PER_PAIR="${1:-20}"
PREFIX="${2:-rl-pilot3}"
PROMOTION_MANIFEST="${3:-}"
OUTPUT="${REPO_ROOT}/arena/out/${PREFIX}_full_field_${SEEDS_PER_PAIR}.json"

if ! [[ "${SEEDS_PER_PAIR}" =~ ^[1-9][0-9]*$ ]]; then
  echo "seeds per pair must be a positive integer" >&2
  exit 2
fi

mapfile -t ACTIVE_AGENTS < <(
  jq -r '.agents | to_entries[] | select(.value.active == true) | .key' \
    "${LB_ROOT}/data/rankings.json" | sort
)
if [[ -n "${PROMOTION_MANIFEST}" ]]; then
  mapfile -t RL_AGENTS < <(jq -r '.accepted[]' "${PROMOTION_MANIFEST}" | sort)
else
  mapfile -t RL_AGENTS < <(
    find "${LB_ROOT}/agents" -maxdepth 1 -type d -name "${PREFIX}-*" \
      -printf '%f\n' | sort
  )
fi
if [[ "${#RL_AGENTS[@]}" -eq 0 ]]; then
  echo "No selected ${PREFIX} agents found." >&2
  exit 3
fi
for agent_name in "${RL_AGENTS[@]}"; do
  if [[ ! -d "${LB_ROOT}/agents/${agent_name}" ]]; then
    echo "Selected agent directory does not exist: ${LB_ROOT}/agents/${agent_name}" >&2
    exit 4
  fi
done

echo "[LocalLB field] active=${#ACTIVE_AGENTS[@]} rl=${#RL_AGENTS[@]} "
echo "[LocalLB field] seeds_per_pair=${SEEDS_PER_PAIR} seat_swap=true output=${OUTPUT}"
if [[ -n "${PROMOTION_MANIFEST}" ]]; then
  echo "[LocalLB field] promoted roster=${PROMOTION_MANIFEST}"
fi
cd "${REPO_ROOT}"
python arena/arena.py \
  --lb-root "${LB_ROOT}" \
  --agents "${ACTIVE_AGENTS[@]}" "${RL_AGENTS[@]}" \
  --seeds-per-pair "${SEEDS_PER_PAIR}" \
  --seed-base 51000 \
  --episode-steps 720 \
  --procs 8 \
  --cores-per-worker 1 \
  --out "${OUTPUT}"

echo "[LocalLB field complete] ${OUTPUT}"
