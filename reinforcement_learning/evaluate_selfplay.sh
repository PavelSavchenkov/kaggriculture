#!/usr/bin/env bash
# Export best self-play checkpoints, validate them, and run a fresh LocalLB field.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LB_ROOT="${KAGGRICULTURE_LB_ROOT:-$(dirname "${REPO_ROOT}")/kaggriculture-localLB}"
SELFPLAY_ROOT="${SELFPLAY_ROOT:-${REPO_ROOT}/reinforcement_learning/artifacts/selfplay-v1}"
PROMOTION_MANIFEST="${PROMOTION_MANIFEST:-${REPO_ROOT}/reinforcement_learning/artifacts/seeds-pilot-elo-v2/promoted_agents.json}"
AGENT_PREFIX="${SELFPLAY_AGENT_PREFIX:-rlsp1}"
SEEDS_PER_PAIR="${LOCAL_LB_SEEDS_PER_PAIR:-20}"
SEED_BASE="${LOCAL_LB_SEED_BASE:-730000}"
PROCESSES="${LOCAL_LB_PROCESSES:-8}"
PREFLIGHT_ONLY="${PREFLIGHT_ONLY:-0}"
EXPORT_MAP="${SELFPLAY_ROOT}/evaluation_exports.json"
ARENA_OUTPUT="${ARENA_OUTPUT:-${REPO_ROOT}/arena/out/${AGENT_PREFIX}_fresh_${SEEDS_PER_PAIR}.json}"
SUMMARY_OUTPUT="${SUMMARY_OUTPUT:-${SELFPLAY_ROOT}/localLB_evaluation.json}"

positive_integer() {
  local name="$1"
  local value="$2"
  if [[ ! "$value" =~ ^[1-9][0-9]*$ ]]; then
    echo "$name must be a positive integer; got '$value'" >&2
    exit 2
  fi
}

positive_integer LOCAL_LB_SEEDS_PER_PAIR "$SEEDS_PER_PAIR"
positive_integer LOCAL_LB_SEED_BASE "$SEED_BASE"
positive_integer LOCAL_LB_PROCESSES "$PROCESSES"
if [[ ! "$AGENT_PREFIX" =~ ^[a-z0-9][a-z0-9_-]{1,20}$ ]]; then
  echo "SELFPLAY_AGENT_PREFIX must be 2-21 lowercase ID characters" >&2
  exit 2
fi
for required in "$PROMOTION_MANIFEST" "$LB_ROOT/config/leaderboard.yaml" "$LB_ROOT/data/rankings.json"; do
  if [[ ! -f "$required" ]]; then
    echo "missing required file: $required" >&2
    exit 2
  fi
done

mapfile -t REPORTS < <(
  find "$SELFPLAY_ROOT" -mindepth 2 -maxdepth 2 -type f \
    -name training_report.json -print | sort
)
mapfile -t EXPECTED_PARENTS < <(jq -r '.accepted[]' "$PROMOTION_MANIFEST" | sort)
expected="${#EXPECTED_PARENTS[@]}"
if [[ "${#REPORTS[@]}" -ne "$expected" ]]; then
  echo "Expected $expected completed lineage reports under $SELFPLAY_ROOT; found ${#REPORTS[@]}." >&2
  exit 3
fi
mapfile -t REPORT_PARENTS < <(
  for report in "${REPORTS[@]}"; do
    jq -r '.arguments.learner_agent' "$report"
  done | sort
)
if [[ "$(printf '%s\n' "${REPORT_PARENTS[@]}")" != "$(printf '%s\n' "${EXPECTED_PARENTS[@]}")" ]]; then
  echo "Self-play reports do not match the promoted parent roster." >&2
  echo "Expected: ${EXPECTED_PARENTS[*]}" >&2
  echo "Found:    ${REPORT_PARENTS[*]}" >&2
  exit 3
fi
for report in "${REPORTS[@]}"; do
  checkpoint="$(jq -r '.clusters[] | .checkpoint' "$report")"
  if [[ ! -f "$checkpoint" ]]; then
    echo "selected best checkpoint is missing: $checkpoint" >&2
    exit 3
  fi
  completed="$(jq '.clusters[] | .completed_updates' "$report")"
  planned="$(jq '.arguments.updates' "$report")"
  if [[ "$completed" -lt "$planned" ]]; then
    echo "lineage is still incomplete: $report ($completed/$planned updates)" >&2
    exit 3
  fi
done

echo "[self-play evaluation preflight] lineages=${#REPORTS[@]} seeds_per_pair=$SEEDS_PER_PAIR seed_base=$SEED_BASE"
echo "[self-play evaluation preflight] output=$ARENA_OUTPUT"
if [[ "$PREFLIGHT_ONLY" == "1" ]]; then
  echo "[self-play evaluation] preflight complete; no exports or games were run"
  exit 0
fi

cd "$REPO_ROOT"
temporary_rows="$(mktemp "${SELFPLAY_ROOT}/.evaluation_exports.XXXXXX")"
trap 'rm -f "$temporary_rows"' EXIT

echo "[self-play evaluation 1/4] export selected best checkpoints"
for index in "${!REPORTS[@]}"; do
  report="${REPORTS[$index]}"
  run_dir="$(dirname "$report")"
  parent="$(jq -r '.arguments.learner_agent' "$report")"
  echo "[export descendants] $((index + 1))/${#REPORTS[@]} parent=$parent"
  PYTHONPATH=reinforcement_learning/python \
    conda run --no-capture-output -n kaggle python \
      reinforcement_learning/tools/export_seed_agents.py \
      --input "$run_dir" --output-root "$LB_ROOT/agents" \
      --agent-prefix "$AGENT_PREFIX" --overwrite
  descendant="$(jq -r '.agents | keys | .[0]' "$LB_ROOT/agents/seed_export_report.json")"
  jq -n \
    --arg parent "$parent" \
    --arg descendant "$descendant" \
    --arg report "$report" \
    --arg checkpoint "$(jq -r '.clusters[] | .checkpoint' "$report")" \
    --argjson best_update "$(jq '.clusters[] | .best_evaluation.update' "$report")" \
    --argjson internal_score "$(jq '.clusters[] | .best_evaluation.score' "$report")" \
    --argjson internal_margin "$(jq '.clusters[] | .best_evaluation.mean_money_margin' "$report")" \
    '{parent_agent:$parent, descendant_agent:$descendant,
      training_report:$report, best_checkpoint:$checkpoint,
      best_update:$best_update, internal_score:$internal_score,
      internal_mean_money_margin:$internal_margin}' >> "$temporary_rows"
done
jq -s --arg root "$SELFPLAY_ROOT" \
  '{format:1, selfplay_root:$root, descendants:.}' "$temporary_rows" > "${EXPORT_MAP}.tmp"
mv "${EXPORT_MAP}.tmp" "$EXPORT_MAP"

mapfile -t DESCENDANTS < <(jq -r '.descendants[].descendant_agent' "$EXPORT_MAP")
echo "[self-play evaluation 2/4] validate exported LocalLB submissions"
for index in "${!DESCENDANTS[@]}"; do
  agent="${DESCENDANTS[$index]}"
  echo "[validate descendants] $((index + 1))/${#DESCENDANTS[@]} $agent"
  PYTHONPATH="$LB_ROOT/src" conda run --no-capture-output -n kaggle python \
    -m lb.cli validate --config "$LB_ROOT/config/leaderboard.yaml" \
    --agent-dir "$LB_ROOT/agents/$agent"
done

mapfile -t ROSTER < <(
  {
    jq -r '.agents | to_entries[] | select(.value.active == true) | .key' \
      "$LB_ROOT/data/rankings.json"
    jq -r '.accepted[]' "$PROMOTION_MANIFEST"
    jq -r '.descendants[].descendant_agent' "$EXPORT_MAP"
  } | sort -u
)
for agent in "${ROSTER[@]}"; do
  if [[ ! -f "$LB_ROOT/agents/$agent/main.py" ]]; then
    echo "arena roster agent is missing main.py: $LB_ROOT/agents/$agent" >&2
    exit 4
  fi
done

echo "[self-play evaluation 3/4] fresh full-horizon, seat-swapped LocalLB field"
echo "[LocalLB roster] agents=${#ROSTER[@]} descendants=${#DESCENDANTS[@]} parents=$expected"
python arena/arena.py \
  --lb-root "$LB_ROOT" --agents "${ROSTER[@]}" \
  --seeds-per-pair "$SEEDS_PER_PAIR" --seed-base "$SEED_BASE" \
  --episode-steps 720 --procs "$PROCESSES" --cores-per-worker 1 \
  --out "$ARENA_OUTPUT"
if ! jq -e '.forfeits == 0' "$ARENA_OUTPUT" >/dev/null; then
  echo "LocalLB evaluation contained forfeits; refusing to certify the result" >&2
  exit 5
fi

echo "[self-play evaluation 4/4] compare every descendant with its parent"
conda run --no-capture-output -n kaggle python \
  reinforcement_learning/tools/summarize_selfplay_arena.py \
  --arena "$ARENA_OUTPUT" --exports "$EXPORT_MAP" --output "$SUMMARY_OUTPUT"
echo "[self-play evaluation complete] $SUMMARY_OUTPUT"
