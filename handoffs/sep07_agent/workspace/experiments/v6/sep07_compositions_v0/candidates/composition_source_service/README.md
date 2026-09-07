# Recorded service compiler variant

Uses the same composition, layout, workforce and market controller as
composition_greedy_v0, with observed daily service masks from its current-leader
source replay. Worker routes are compiled afresh. Source masks cover water,
fertilizer, feed, care, fertilizer collection and harvest. Fertilizing before
watering is an explicit dependency for one-shot crop yield. Missed feeding can
trigger a survival repair. Terminal harvest/collection remains allowed.

The shared C++ compiler is in the neighboring composition_greedy_v0 package;
all dependencies are within this experiment or persistent root API/engine.
Exact source lineage is its PROGRAM_SOURCES.json, program 0, episode 106277936
seat 0. This tests whether service-template fidelity closes execution loss;
it is not a promoted agent or a claim that the source masks are always optimal.
