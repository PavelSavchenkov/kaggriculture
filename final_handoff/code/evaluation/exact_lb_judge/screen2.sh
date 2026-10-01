#!/bin/bash
# LB screen driver v2 (Imitation's order after its G3 pre-screen): starts each arm when fewer than MAXRUN lbrun.sh screen runs are active
# machine-wide (counts all 'lbrun.sh scr_' runs, incl. those of other drivers). usage: [MAXRUN=3] screen2.sh <agent name> ...
cd "$(dirname "$0")"
SPECS="pavel-bc-opus-v17d-dc12m19-fc2-m68:977000:15 pavel-bc-opus-v17d-dc12m14-fc2-m68:979000:15 pavel-mmpq-policy-v2:982000:15"
nrun() { ps -eo args | grep -c "^/bin/bash ./lbrun.sh scr_"; }
for a in "$@"; do
  while [ $(nrun) -ge ${MAXRUN:-3} ]; do sleep 20; done
  RUNQ_PRIO=hi P=9 ./lbrun.sh scr_$a agents/$a $SPECS >> out/screen.log 2>&1 &
  sleep 10
done
wait; echo "screen2 done: $*" >> out/screen.log
