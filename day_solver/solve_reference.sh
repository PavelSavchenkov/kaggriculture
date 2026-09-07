#!/usr/bin/env bash
set -euo pipefail
if (( $# < 2 || $# > 4 )); then
    echo "Usage: $0 public-v3.json new-output-directory [seconds=900] [fallback-threads=8]" >&2
    exit 2
fi
release_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$release_dir/runtime/lib"
exec "$release_dir/runtime/reference_solver_cli" "$1" "$2" \
    --iterations 64 --variants 8 --rounds 2 --fallback 1 --compact 1 --exact-tuning 2 --screen-tuning 2 --route-seconds 0.15 --completion-seconds 1.5 --screen-seconds 0.9 --retry-seconds 4 \
    --time-limit "${3:-900}" --exact-workers "${4:-8}"
