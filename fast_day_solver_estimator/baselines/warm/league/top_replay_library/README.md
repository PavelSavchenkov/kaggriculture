# Current top-player replay courses

Ninety fixed observed courses from September 7 strong-player replay cohorts.
Programs 0–71 match the original composition library; programs 72–89 append
six recent games each from three new top-12 teams in the 03:14 leaderboard.
Every episode, seat, submission,
leaderboard snapshot and replay hash is recorded in IMPORT.json. All worker
actions and market orders are copied directly; active worker count is normalized.
Engine-ignored numeric arguments on PASS/HIRE/BUY_LAND are normalized to zero.
No original branching logic is inferred. Default program 0 is the earlier rank-1 course
also isolated in leader_program0_tape. No policy optimization or runtime repairs.

Agent constructor selects a program before the game starts. Each instance owns
its selection; immutable numeric tables are shared. This supports exact
same-composition calibration and provides current-player league candidates.
