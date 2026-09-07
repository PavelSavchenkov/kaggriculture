#!/usr/bin/env bash
set -euo pipefail
if (( $# == 0 )); then
    echo "Usage: $0 executable [arguments...]" >&2
    exit 2
fi
day_solver_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$day_solver_dir/runtime/lib"
exec "$@"
