#!/bin/bash
# Team CPU gate: every game / screen process runs through here. At most N=18 single-threaded compute processes run on the machine
# (i7-14700K: 20 physical cores / 28 threads; 2 cores kept for GPU-training feeders, builds and the sessions). Slots 1-4 are reserved for
# priority "hi" (Stage-1 screens); "lo" (Stage-2 games) uses slots 5-18. A job waits (sleeping, no CPU) until a slot is free.
# usage: slot.sh [-p hi|lo] [-n k] -t <tag> <command ...>     (k = threads the command uses; default 1)
#   env RUNQ_SESSION=<Imitation|BC|DayCompiler|Weaknesses|...> identifies the owner in `runq/status.sh`.
# First come, first served within a priority (a ticket per job): when k slots are free, the first k waiters may start, so a launcher's
# games start in list order and fixed-cohort reads (seqtest --order) fill in order. Waiting "hi" jobs go before waiting "lo" jobs while
# "hi" holds < HI_MAX slots; hi never holds more than HI_MAX slots outside the reserved ones, so Stage 2 keeps >= N - HI_MAX.
# A job whose tag has a stop file (runq/stop/<tag>, written by seqwatch.sh at a verdict) exits 0 without running.
# Never edit this file in place while jobs run (bash reads scripts incrementally): write a new file and mv it.
export LC_ALL=C
R=/home/pavel/Programming/kaggriculture/work/runq; N=18; RES=4; HI_MAX=8
prio=lo; k=1; tag=untagged
while [ $# -gt 0 ]; do case $1 in -p) prio=$2; shift 2;; -n) k=$2; shift 2;; -t) tag=$2; shift 2;; --) shift; break;; *) break;; esac; done
stopped() { [ -e "$R/stop/${tag//\//_}" ]; }
stopped && exit 0
first=$([ "$prio" = hi ] && echo 1 || echo $((RES + 1)))
exec {tk}>>$R/locks/ticket.lock; flock $tk
ticket=$(( $(cat $R/ticket 2>/dev/null || echo 0) + 1 )); echo $ticket > $R/ticket
flock -u $tk; exec {tk}>&-
qp=$([ "$prio" = hi ] && echo hq || echo lq)  # v8 prefixes (older waiters used hi / lo)
me=$R/waiting/$qp.$(printf %010d $ticket).$$
touch $me
trap 'rm -f $me' EXIT
trap 'exit 143' TERM INT
held() {  # $1 = hi | range: live holders of slots (hi: priority-hi holders anywhere; range: slots first..N held by anyone)
  local n=0 i t s p tg x pid start=1
  [ "$1" = range ] && start=$first  # (a bash arithmetic "$1 == range" compared two unset names: always true; fixed in v7)
  for ((i = start; i <= N; ++i)); do
    read -r t s p tg x pid < $R/running/slot$i 2>/dev/null || continue
    [ -n "$pid" ] && kill -0 $pid 2>/dev/null || continue
    [ $1 = hi ] && [ "$p" != hi ] && continue
    n=$((n + 1))
  done
  echo $n
}
rank() {  # number of waiting files of my priority ahead of mine (glob order = ticket order under LC_ALL=C)
  local n=0 w
  for w in $R/waiting/$qp.*; do [ "$w" = "$me" ] && break; n=$((n + 1)); done
  echo $n
}
clean_ahead() {  # drop waiting files of dead jobs ahead of mine (SIGKILLed waiters)
  local w
  for w in $R/waiting/*.*; do [ "$w" = "$me" ] && break; kill -0 ${w##*.} 2>/dev/null || rm -f "$w"; done
}
got=()
grab() {  # one free slot in [first, last] -> got+=(i), fd kept open (the lock is held until this script exits)
  local last=$N
  if [ "$prio" = hi ] && [ $(held hi) -ge $HI_MAX ]; then  # hi at its cap: only the reserved slots while lo jobs wait (astra other-014)
    local w; for w in $R/waiting/lq.* $R/waiting/lo.*; do [ -e "$w" ] && { last=$RES; break; }; done  # no lo waiting: work-conserving
  fi
  for i in $(seq $first $last); do
    exec {fd}>>$R/locks/slot$i.lock
    if flock -n $fd; then got+=($i); eval "fd_$i=$fd"; return 0; fi
    exec {fd}>&-
  done
  return 1
}
polls=0
while [ ${#got[@]} -lt $k ]; do
  stopped && exit 0
  free=$(( N - first + 1 - $(held range) ))
  go=0
  if [ $free -gt 0 ] && [ $(rank) -lt $free ]; then
    go=1
    if [ "$prio" = lo ]; then  # yield to waiting hi jobs while hi holds < HI_MAX
      hw=( $R/waiting/hq.* ); [ -e "${hw[0]}" ] && [ $(held hi) -lt $HI_MAX ] && [ $free -le ${#hw[@]} ] && go=0
    fi
  fi
  if [ $go = 1 ] && grab; then continue; fi
  polls=$((polls + 1)); [ $((polls % 40)) = 0 ] && clean_ahead
  sleep 0.5
done
rm -f $me
stopped && exit 0
for i in "${got[@]}"; do echo "$(date '+%H:%M:%S') ${RUNQ_SESSION:-unknown} $prio $tag pid $$" > $R/running/slot$i; done
"$@"; rc=$?
for i in "${got[@]}"; do : > $R/running/slot$i; done
exit $rc
