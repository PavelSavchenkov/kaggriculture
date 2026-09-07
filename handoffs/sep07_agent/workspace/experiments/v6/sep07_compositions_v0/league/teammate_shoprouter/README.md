# Teammate shoprouter v2

C++ local API adapter for `external/kaggriculture/agents/shoprouter-rl-v2`.
The original yhay81 production backbone and teammate additive market head are
preserved. `NOTICE` and `SOURCE_SHA256SUMS` identify the sources. Public rating
is unknown. Reported upstream local/live results are not new validation.

The adapter receives only legal observations and reconstructs a sanitized state
for the borrowed code. It clamps returned unit count to active workers and
finalizes action metadata. Borrowed tapes are parsed once as immutable data.
The additive selling head is translated from Python. It matches 7,190 original
Python/native reference actions across eight recorded and two forced-route
sequences, after hydrating replay-shared step and normalizing inactive arguments.
