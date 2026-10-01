#!/bin/bash
# Follow-up for an arm whose stage-1 lbrun is running without its screen3 pipeline: wait for that lbrun to exit, gate check, then
# stage 2 on PASS (same as screen3.sh arm()). usage: stage2.sh <arm> <stage-1 lbrun pid>
cd "$(dirname "$0")"; a=$1
S1="pavel-bc-opus-v17d-dc12m19-fc2-m68:977000:15"; S2="pavel-bc-opus-v17d-dc12m14-fc2-m68:979000:15 pavel-mmpq-policy-v2:982000:15"
while kill -0 $2 2>/dev/null; do sleep 20; done
g=$(conda run --no-capture-output -n kaggriculture python gatecheck.py scr_$a 2>&1 | grep -E "^(PASS|FAIL)"); echo "$(date '+%H:%M') $g" >> out/gate.log
case "$g" in PASS*) RUNQ_PRIO=hi P=10 ./lbrun.sh scr_$a agents/$a $S1 $S2 >> out/screen.log 2>&1; echo "$(date '+%H:%M') full done scr_$a" >> out/gate.log;; esac
