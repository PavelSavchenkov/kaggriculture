#!/bin/bash
# Auto-launch BC's f5_mix (sep29_bc_mm/models/lb1/f5_mix) straight to 90 (Imitation's priority 2) once MANIFEST.txt exists and files are stable.
cd "$(dirname "$0")"; M=/home/pavel/Programming/kaggriculture/experiments/v10/sep29_bc_mm/models/lb1/f5_mix
snap() { (cd $M && find . -type f -printf '%p %s %T@\n' | sort) 2>/dev/null; }
until [ -f $M/MANIFEST.txt ]; do sleep 30; done; a=$(snap); sleep 60; until [ "$a" = "$(snap)" ]; do a=$(snap); sleep 60; done
./mkagent.sh bc_f5_mix $M >> out/screen.log 2>&1
A=/home/pavel/Programming/kaggriculture/experiments/v10/sep24_BC_opus/data/localLB_d6157df/agents/pavel-bc-opus-v17d-dc12m19-fc2-m68/model
echo "$(date '+%H:%M') f5_mix differs from #1: $(diff -rq $M $A 2>&1 | sed 's|.*/||' | grep -v MANIFEST | tr '\n' ' ')" >> out/gate.log
RUNQ_PRIO=hi P=12 ./lbrun.sh scr_bc_f5_mix agents/bc_f5_mix pavel-bc-opus-v17d-dc12m19-fc2-m68:977000:15 pavel-bc-opus-v17d-dc12m14-fc2-m68:979000:15 pavel-mmpq-policy-v2:982000:15 >> out/screen.log 2>&1
echo "$(date '+%H:%M') full done scr_bc_f5_mix" >> out/gate.log
