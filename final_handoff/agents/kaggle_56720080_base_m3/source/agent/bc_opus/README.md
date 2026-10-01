# bc_opus

BC network emits one DayIntent per dawn (designs/day_intent.md); the day compiler
in `source/compiler.cpp` executes it (designs/day_compiler.md). Model weights:
`BC_OPUS_MODEL` or the build default `models/selected/model.bin`.

Inputs are the dawn observation, the DayIntent schema, and opponent market flow
inferred from the agent's own observations. Decoding never emits fixed values and
enforces partitions and bounds; the compiler checks site capacity.
