# Runtime guard miss: extra weed on a planned wheat tile

The diagnostic driver reproduces both full-game action hashes for
animal_groups_m1 versusKingRC4, seed1014seat1. The first mismatch isday23.
Cow calendar0 expects tiles38and49empty; calendar1 already handles weed49.
The actual farm has weeds on both38and49. Continuing calendar1 therefore
misses the planting on38. Later guards remain different because that wheat
crop is missing. This costs six more wheat than the intended crop replacement.

The three new cows still produce the intended45milk and39fertilizer, while
own cash improves8088(mode1)or8261(mode2). This case is profitable but exposes
incomplete crop repair. A third observed-state day23 program, or a general
feasible insertion of DIG before PLANT, can preserve that lost wheat.
Required proof: exact daily endpoint and719-turn paired cash/production,
then ordinary guards and native/fresh scenarios. Do not blindly insert DIG
in place of PLANT and lose watering or later animal service.

Source: source/trace_guard.cpp; exact diagnostic output:GUARD_TRACE.txt;
original per-game records:study/animal_groups_m1_vs_king_rc4.json andmode2.
