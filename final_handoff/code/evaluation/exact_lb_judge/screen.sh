#!/bin/bash
# LB fast screen (user direction Sep 30 ~21:00): each challenger folder in agents/ vs m19-fc2 (#1), m14-fc2 (live) and pavel-mmpq-policy-v2
# on the seed blocks a new challenger gets (lb_new_pairs.py on localLB_d6157df: 977000 / 979000 / 982000; the RL agent's block 978000 is
# skipped by the user's no-RL rule), 15 seeds x 2 seats = 90 games; hi priority; at most 2 arms at a time (queue order = argument order).
# usage: screen.sh <agent name> ...
cd "$(dirname "$0")"
SPECS="pavel-bc-opus-v17d-dc12m19-fc2-m68:977000:15 pavel-bc-opus-v17d-dc12m14-fc2-m68:979000:15 pavel-mmpq-policy-v2:982000:15"
for a in "$@"; do
  while [ $(jobs -r | wc -l) -ge 2 ]; do sleep 20; done
  RUNQ_PRIO=hi P=9 ./lbrun.sh scr_$a agents/$a $SPECS >> out/screen.log 2>&1 &
  sleep 5
done
wait; echo "screen done: $*" >> out/screen.log
