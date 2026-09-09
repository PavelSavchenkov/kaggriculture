def copy_ceiling(best_tape_score, recovery, target):
    """Can replaying tapes reach `target`?

    best_tape_score : the highest score any team you can copy actually holds
    recovery        : fraction of a tape owner's score your replay reproduces
    target          : the score you are aiming at, e.g. the gold cutoff
    """
    reachable = best_tape_score * recovery
    needed_owner = target / recovery
    needed_recovery = target / best_tape_score
    return {
        "best you can reach by copying": round(reachable, 1),
        "short of target by": round(target - reachable, 1),
        "owner score you would need": round(needed_owner, 1),
        "recovery you would need": round(needed_recovery, 3),
        "possible": reachable >= target,
    }


# Kaggriculture, 2026-09-01 standings
GOLD, BEST_TAPE = 2746.3, 2914.5   # public LB, 8 September
RECOVERY = (0.920 + 0.848) / 2          # the two replays I measured

for k, v in copy_ceiling(BEST_TAPE, RECOVERY, GOLD).items():
    print(f"  {k:32s} {v}")

print()
print("  where I am: 2527.7, rank 271 of 8151")
print("  so the whole copy-a-tape family tops out ~170 points under gold,")
print("  and no choice of tape changes that.")
