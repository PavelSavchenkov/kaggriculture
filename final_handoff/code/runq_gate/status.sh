#!/bin/bash
# Who holds the team's CPU slots now (work/runq/slot.sh), per session and tag; machine load; stopped tags; recent verdicts.
R=/home/pavel/Programming/kaggriculture/work/runq
live=$(for f in $R/running/slot*; do read -r t s p tag _ pid < $f 2>/dev/null && kill -0 $pid 2>/dev/null && echo "$s $p $tag"; done)
echo "load: $(cut -d' ' -f1-3 /proc/loadavg)   slots held: $(echo -n "$live" | grep -c .) / 18   waiting jobs: $(pgrep -c -f '^/bin/bash .*runq/slot.sh') total slot.sh"
echo "$live" | grep . | sort | uniq -c | sort -rn
ls $R/stop 2>/dev/null | sed 's/^/stopped: /'
tail -5 $R/verdicts.log 2>/dev/null
true
