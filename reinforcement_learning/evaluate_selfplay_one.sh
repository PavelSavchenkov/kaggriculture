#!/usr/bin/env bash
# Export and test one completed self-play descendant against the active LocalLB field.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LB_ROOT="${KAGGRICULTURE_LB_ROOT:-$(dirname "$REPO_ROOT")/kaggriculture-localLB}"
SELFPLAY_ROOT="${SELFPLAY_ROOT:-$REPO_ROOT/reinforcement_learning/artifacts/selfplay-v1}"
AGENT_PREFIX="${SELFPLAY_AGENT_PREFIX:-rlsp-live}"
SEEDS_PER_PAIR="${LOCAL_LB_SEEDS_PER_PAIR:-20}"
SEED_BASE="${LOCAL_LB_SEED_BASE:-830000}"
PROCESSES="${LOCAL_LB_PROCESSES:-8}"
REQUESTED="${1:-}"

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

choose_report() {
  local candidate completed planned
  if [[ -n "$REQUESTED" ]]; then
    if [[ -d "$REQUESTED" ]]; then
      candidate="$REQUESTED/training_report.json"
    else
      candidate="$SELFPLAY_ROOT/$REQUESTED/training_report.json"
    fi
    if [[ ! -f "$candidate" ]]; then
      echo "No self-play training report found for: $REQUESTED" >&2
      return 1
    fi
    printf '%s\n' "$candidate"
    return
  fi
  while IFS= read -r candidate; do
    completed="$(jq '.clusters[] | .completed_updates' "$candidate")"
    planned="$(jq '.arguments.updates' "$candidate")"
    if [[ "$completed" -ge "$planned" ]]; then
      printf '%s\n' "$candidate"
      return
    fi
  done < <(find "$SELFPLAY_ROOT" -mindepth 2 -maxdepth 2 -type f \
            -name training_report.json -print | sort)
  echo "No fully trained self-play lineage is available under $SELFPLAY_ROOT" >&2
  return 1
}

REPORT="$(choose_report)"
RUN_DIR="$(dirname "$REPORT")"
PARENT="$(jq -r '.arguments.learner_agent' "$REPORT")"
COMPLETED="$(jq '.clusters[] | .completed_updates' "$REPORT")"
PLANNED="$(jq '.arguments.updates' "$REPORT")"
BEST_UPDATE="$(jq '.clusters[] | .best_evaluation.update' "$REPORT")"
BEST_CHECKPOINT="$(jq -r '.clusters[] | .checkpoint' "$REPORT")"
if [[ "$COMPLETED" -lt "$PLANNED" ]]; then
  echo "Selected lineage is incomplete: $COMPLETED/$PLANNED updates" >&2
  exit 3
fi
for required in "$BEST_CHECKPOINT" "$LB_ROOT/config/leaderboard.yaml" \
                "$LB_ROOT/data/rankings.json"; do
  if [[ ! -f "$required" ]]; then
    echo "Missing required file: $required" >&2
    exit 3
  fi
done

cd "$REPO_ROOT"
echo "[individual evaluation 1/4] export best checkpoint parent=$PARENT update=$BEST_UPDATE"
PYTHONPATH=reinforcement_learning/python \
  conda run --no-capture-output -n kaggle python \
    reinforcement_learning/tools/export_seed_agents.py \
    --input "$RUN_DIR" --output-root "$LB_ROOT/agents" \
    --agent-prefix "$AGENT_PREFIX" --overwrite
DESCENDANT="$(jq -r '.agents | keys | .[0]' "$LB_ROOT/agents/seed_export_report.json")"

echo "[individual evaluation 2/4] validate LocalLB agent=$DESCENDANT"
PYTHONPATH="$LB_ROOT/src" conda run --no-capture-output -n kaggle python \
  -m lb.cli validate --config "$LB_ROOT/config/leaderboard.yaml" \
  --agent-dir "$LB_ROOT/agents/$DESCENDANT"

mapfile -t ROSTER < <(
  {
    jq -r '.agents | to_entries[] | select(.value.active == true) | .key' \
      "$LB_ROOT/data/rankings.json"
    printf '%s\n' "$PARENT" "$DESCENDANT"
  } | sort -u
)
for agent in "${ROSTER[@]}"; do
  if [[ ! -f "$LB_ROOT/agents/$agent/main.py" ]]; then
    echo "LocalLB roster agent is missing main.py: $LB_ROOT/agents/$agent" >&2
    exit 4
  fi
done

ARENA_OUTPUT="${ARENA_OUTPUT:-$RUN_DIR/${DESCENDANT}_locallb.json}"
EXPORT_MAP="$RUN_DIR/${DESCENDANT}_evaluation_export.json"
SUMMARY_OUTPUT="${SUMMARY_OUTPUT:-$RUN_DIR/${DESCENDANT}_locallb_summary.json}"
jq -n \
  --arg parent "$PARENT" --arg descendant "$DESCENDANT" \
  --arg report "$REPORT" --arg checkpoint "$BEST_CHECKPOINT" \
  --argjson best_update "$BEST_UPDATE" \
  '{format:1, descendants:[{parent_agent:$parent,
    descendant_agent:$descendant, training_report:$report,
    best_checkpoint:$checkpoint, best_update:$best_update}]}' > "${EXPORT_MAP}.tmp"
mv "${EXPORT_MAP}.tmp" "$EXPORT_MAP"

echo "[individual evaluation 3/4] challenger-only field agents=${#ROSTER[@]} seeds_per_pair=$SEEDS_PER_PAIR"
python arena/arena.py --lb-root "$LB_ROOT" --agents "${ROSTER[@]}" \
  --challenger "$DESCENDANT" --seeds-per-pair "$SEEDS_PER_PAIR" \
  --seed-base "$SEED_BASE" --episode-steps 720 \
  --procs "$PROCESSES" --cores-per-worker 1 --out "$ARENA_OUTPUT"
if ! jq -e '.forfeits == 0' "$ARENA_OUTPUT" >/dev/null; then
  echo "Evaluation contained forfeits; refusing to certify the result" >&2
  exit 5
fi

echo "[individual evaluation 4/4] summarize descendant and parent"
conda run --no-capture-output -n kaggle python \
  reinforcement_learning/tools/summarize_selfplay_arena.py \
  --arena "$ARENA_OUTPUT" --exports "$EXPORT_MAP" --output "$SUMMARY_OUTPUT"
echo "[individual evaluation complete] $SUMMARY_OUTPUT"
