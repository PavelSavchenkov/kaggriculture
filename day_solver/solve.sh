#!/usr/bin/env bash
set -euo pipefail
if (( $# < 2 || $# > 5 )); then
    echo "Usage: $0 public-v3.json new-output-directory [seconds=900] [fallback-threads=8] [portfolio|regret|regret-deferred|regret-fast]" >&2
    exit 2
fi
release_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$release_dir/runtime/lib"
exec "$release_dir/runtime/day_solver_cli" "$1" "$2" "${3:-900}" "${4:-8}" "${5:-portfolio}"
