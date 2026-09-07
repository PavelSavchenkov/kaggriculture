# One random shop dynamic robust

Observation-only policy for the 30-day game with a PASS opponent and only the
first day-3 random shop enabled. Before reveal it executes a shared Bakery
opening while advancing all eight specialist policy states on the real
observations. After reveal it continues the matching warmed specialist and
applies promoted shop-specific market and route repairs.

The agent assumes the revealed shop is one of the eight standard shops. It uses
no seed, future randomness, simulator state, or opponent-private information.
All specialist sources and tapes are vendored here so this official agent is
self-contained.
