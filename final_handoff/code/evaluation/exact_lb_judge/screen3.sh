#!/bin/bash
# LB screen v3 (Imitation's decision 21:45): per arm, stage 1 = 30 games vs #1 (977000:15, both seats); stage 2 = vs m14-fc2 979000 and
# mmpq 982000 (60) only if gatecheck.py PASSes; arms named with suffix :1 run stage 1 only. At most MAXARM arm pipelines at once
# (machine-wide count of 'lbrun.sh scr_' runs). usage: [MAXARM=3] screen3.sh <agent[:1]> ...
cd "$(dirname "$0")"
S1="pavel-bc-opus-v17d-dc12m19-fc2-m68:977000:15"; S2="pavel-bc-opus-v17d-dc12m14-fc2-m68:979000:15 pavel-mmpq-policy-v2:982000:15"
nrun() { ps -eo args | grep -c "^/bin/bash ./lbrun.sh scr_"; }
arm() { a=${1%%:*}; RUNQ_PRIO=hi P=10 ./lbrun.sh scr_$a agents/$a $S1 >> out/screen.log 2>&1
  g=$(conda run --no-capture-output -n kaggriculture python gatecheck.py scr_$a 2>&1 | grep -E "^(PASS|FAIL)"); echo "$(date '+%H:%M') $g" >> out/gate.log
  case "$1" in *:1) return;; esac
  case "$g" in PASS*) RUNQ_PRIO=hi P=10 ./lbrun.sh scr_$a agents/$a $S1 $S2 >> out/screen.log 2>&1; echo "$(date '+%H:%M') full done scr_$a" >> out/gate.log;; esac; }
for x in "$@"; do
  while [ $(nrun) -ge ${MAXARM:-3} ]; do sleep 20; done
  arm $x & sleep 15
done
wait; echo "screen3 done: $*" >> out/screen.log
