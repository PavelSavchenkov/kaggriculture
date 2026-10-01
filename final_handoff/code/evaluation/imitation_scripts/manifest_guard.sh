#!/bin/bash
# Run-identity guard (astra-004): the manifest of a run = sha256 of the binary, of every file of each model dir (links resolved) and the
# env flags that change play. First call writes <file>; a later call (a resume) must produce the same manifest, else it refuses (exit 3).
# usage: manifest_guard.sh <manifest file> <binary> <model dir> [<model dir> ...]
out=$1; bin=$2; shift 2
new=$(mktemp)
{ echo "binary $(sha256sum $(readlink -f $bin) | cut -c1-16)"
  for d in "$@"; do for f in $(find -L $d -maxdepth 2 -type f ! -name MANIFEST.txt | sort); do echo "model $(sha256sum $(readlink -f $f) | cut -c1-16) ${f#$d/} @ $(basename $(readlink -f $d))"; done; done
  env | grep -E '^(DUEL_|DC11_|DC12_|BC_|SHOP_CRN|REPLAY_|OD_|TEACHER_|SELLDIFF_)' | grep -v -E '^(DUEL_BIN|BC_OPUS_MODEL)=' | sort | sed 's/^/env /'; } > $new
if [ -s "$out" ]; then
  if ! diff -q <(grep -v '^time ' "$out") $new > /dev/null; then
    echo "MANIFEST MISMATCH for $out (refusing to resume):" >&2; diff <(grep -v '^time ' "$out") $new | head -10 >&2; exit 3
  fi
else
  mkdir -p "$(dirname "$out")"; { echo "time $(date '+%F %T')"; cat $new; } > "$out"
fi
