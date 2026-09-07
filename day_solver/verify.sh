#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
sha256sum --check --quiet SHA256SUMS
echo "Day solver package checksums match."
