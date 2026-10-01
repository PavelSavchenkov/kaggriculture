# Team compute and verdict rules (Sep 30, user-requested reorganisation)

The machine (i7-14700K: 20 physical cores / 28 threads, RTX 5090) is shared by 4 sessions. Game simulations are single-threaded and
compute-bound: throughput stops growing near one process per physical core; beyond that every run just gets slower.

## 1. One CPU gate for every game / screen process

- Run every game or screen process through `work/runq/slot.sh`: at most 18 single-threaded compute processes on the machine
  (2 cores stay free for GPU-training feeders, builds and the sessions).
  `RUNQ_SESSION=<Imitation|BC|DayCompiler|Weaknesses> work/runq/slot.sh [-p hi|lo] [-n threads] -t <tag> <command ...>`
- `-p hi` = Stage-1 screens (slots 1-18, 4 of them reserved for hi; waiting hi jobs take freed slots before lo jobs while hi holds
  < 8 slots); default `lo` = Stage-2 games (slots 5-18).
- `-n k` for a command using k threads (teacher_day / options_diff / seller_diff with a threads argument, full_games with threads > 1,
  training feeders). Prefer 1 thread per process.
- Your own `xargs -P` can be any size: the gate holds the extra jobs asleep (no CPU) until a slot frees. Jobs start first come,
  first served within a priority (a ticket per job), so a launcher's games start in list order.
- `work/runq/status.sh` shows who holds the slots. Check it (and GATES.md) before launching, to avoid duplicate runs.

## 2. Two stages; Stage 2 stops as early as the data allow

- Stage 1 (seconds to minutes, priority hi): teacher-forced G1 / G2 / seller_diff, own-state intent splits, probes, day-limited
  continuations (DUEL_STOP_DAY / hand-over) on only the days a change touches. Most ideas should die here.
- Stage 2 (priority lo), only for Stage-1 survivors, one candidate per session at a time:
  - Register one line in `work/runq/PLANNED.md` first: tag, owner, bed, baseline, delta (the minimal effect worth having, default
    $300 / game), expected effect, max games.
  - Bed order: G3-wide on the 218 complement (the clean 99 for networks trained on G3 worlds) -> if PROMOTE, the swap bed (234). The
    league is a final lineage report and the 760 a collapse check (<= 48 games) for finalists only.
  - Interleave the arms (world i of both arms together) and read `work/runq/seqtest.py --order <run list> --only <set list>` (fixed cohorts) at 24 / 48 / 96 / 192 paired games:
    REJECT if mean + 1.28 SE < delta, PROMOTE if mean - 2.0 SE > 0 (5 looks), else continue. Stop the run at the first REJECT / PROMOTE.
    (On today's data: v5t and sbg would have been rejected after 24 games, the package funding change promoted after 96.)
  - Report margin AND half-tie score (W / T / L) for bed reads.

- Never edit a launcher (or slot.sh) in place while jobs use it: bash reads scripts incrementally; write a new file and mv it.

## 3. Identity and hygiene (from the morning incidents)

Read-only shared models, run manifests (`experiments/v10/sep29_mm_copy/scripts/manifest_guard.sh`), frozen builds, no `rm` on
variable paths, Imitation integrates combined candidates. Details: work/mm_handoff/PLAN.md "Setup rules".
