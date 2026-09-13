# Materialization regression

Third, exposed game108281701, player0, day15. The public problem and physical source actions are copied here so the test is self-contained.

These actions are a diagnostic witness only. The test extracts their task order and checks whether the timed materializer can construct a valid schedule. They never enter the cold public scheduler or its benchmark coverage.

Before the DROP source/consumer constraints, the coarse model was feasible but materialization failed with “multi-item return retains inputs”. The regression checks a returned schedule with strict physical replay.

Unknown Mother-Goose, game108286602, player1, day25 checks insertion order within worker cargo at night. Item-number order incorrectly retained4extra wool and lost4wheat. Third, game108281701, player0, day28 also checks that a multi-item DROP empties all cargo in the planned inventory model. Each fixture is diagnostic only and copied from exposed development cases.
