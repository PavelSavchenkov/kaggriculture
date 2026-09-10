#!/usr/bin/env bash
# Canonical entry point: exact submission-ID lineages ranked by Kaggle rating.
# The older v2 recipe remains available only for reproducing its team-level run.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/run_pilot_elo.sh" "$@"
