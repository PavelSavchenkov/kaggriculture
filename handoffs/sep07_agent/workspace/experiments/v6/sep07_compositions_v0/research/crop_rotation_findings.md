# Earlier full-output crop rotations

Idea source: muelsyse111/kaggriculture-crop-timing-and-labor-budget, downloaded
2026-09-07 at08:04. Its melon timing observation was checked against the local
official engine. No notebook policy code was copied. The new C++ lifecycle and
service-search code is local; exact source hashes are recorded in the run.

`CountSpan.crop_harvest_age=-2` chooses the earliest age with the same biological
output as the last productive age. The old -1 default is preserved. This handles
unfertilized crops that never reach the nominal cap; it does not assume every
crop reaches its cap early. Ongoing crops keep their last productive age.
`prune_service=true` independently deletes fertilizer, then water visits from
latest to earliest if all dated output and occupancy remain equal. This is a
local search under the dated biology model, not a proof of minimum route cost.

All40 isolated crop/fertilizer/harvest/pruning cases match exact engine output
and successful work on every day. Across those cases78 water visits and5
fertilizer visits are deleted. For melon, earliest full-output harvest is age10
with or without fertilizer. Maintaining two melon tiles over22 days therefore
fits two11-day rotations rather than one13-day rotation followed by an immature
9-day request. The old compiler refuses that immature second planting; realized
occupancy is26 rather than44 crop-days. The earlier rule fulfills all44 and
produces24 rather than12 melons in each of32 exact PASS games, cash6,054→9,008.

Ten unfertilized melon tiles produce60→119 rather than the predicted120;
one realized unit remains missing. Cash17,074→28,122. Optional service pruning
keeps119 output, saves six hires and raises cash to28,128. With the old rotation,
pruning saves four hires and preserves60 output. Neither change causes unit
faults. A zero-fault policy can still miss its production contract.

The fertilized ten-melon control produces only38 rather than predicted60,
buys44 fertilizer, uses30 and resells14. Removing biologically redundant
fertilization restores60 output and removes all fertilizer trades; this is a
real logistics/production improvement as well as a cost saving. Earlier harvest
with fertilizer produces120, cash24,101; earlier harvest plus pruning produces
119, cash28,128. Labor and capital effects must be checked together.

The mixed farm adds four melon tiles to maintained wheat, strawberries, cows
and sheep. Earlier rotations raise melon output24→47.8125, mean cash45,078→
49,006 and PASS J43,982→47,887. Service pruning alone lowers J to42,812;
combining it with early rotations gives47,632, slightly below early rotations
alone. Fewer requested visits and hires do not ensure stronger realization.
Keep early harvest and service pruning as separate compiler choices.

`runs/crop_rotations_001` retains16 complete variants,32 games each, all full
profiles, biological contracts and causal means. Expansion takes roughly1–13
microseconds per proposal in this small audit. These are cold compiler findings;
none of these farms is a proposed replacement for the strong league agents.
Next inspect the missing output and combine improved lifecycle intent with
stronger placement, workforce and route compilation.
