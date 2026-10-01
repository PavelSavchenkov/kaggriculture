#!/bin/sh
# Vendor a dc11 snapshot (Compiler Overhaul session) into dc11/ and agent/bc_overhaul/source/, then re-apply this
# experiment's local additions (scripts/dc11_local_additions.py). source/history.hpp (exact_shed, advance) and
# source/world.hpp (order-size clamp) carry additive local edits and are exempt from the FROZEN_DEPS check, as is
# agent/bc_opus/source/agent.cpp. Local additions to the agent (<model>.ensemble, one-dawn decode push,
# <model>.ensemble_opening). Checks the snapshot's FROZEN_DEPS against source/ first.
# usage: scripts/vendor_dc11.sh <snapshot dir, e.g. ../sep25_compiler_overhaul/snapshots/v29>
set -e
cd "$(dirname "$0")/.."
snap=$1
awk '{print $2}' $snap/FROZEN_DEPS.sha256 | while read f; do
    a=$(grep " $f\$" $snap/FROZEN_DEPS.sha256 | cut -c1-64); b=$(sha256sum $f | cut -c1-64)
    [ "$a" = "$b" ] || [ "$f" = agent/bc_opus/source/agent.cpp ] || [ "$f" = agent/bc_opus/source/agent.hpp ] || [ "$f" = source/history.hpp ] || [ "$f" = source/world.hpp ] || { echo "dependency differs: $f"; exit 1; }
done
cp -f $snap/dc11/* dc11/
cp -f $snap/agent/bc_overhaul/source/* agent/bc_overhaul/source/
cp -f $snap/README.md dc11/SNAPSHOT_README.md
conda run -n kaggriculture python scripts/dc11_local_additions.py
{
    echo "# dc11 (vendored)"; echo
    echo "Copied on $(date '+%b %d %H:%M') from $snap (Compiler Overhaul session) with scripts/vendor_dc11.sh; local additions"
    echo "to agent/bc_overhaul (off by default): <model>.ensemble, one-dawn decode push, <model>.ensemble_opening."
    echo; echo "Snapshot SHA256:"; cat $snap/SHA256
} > dc11/VENDORED.md
echo "vendored $snap"
