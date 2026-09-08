# Cow-placement notebook inspection

Source: dmitriigluzdov/kaggriculture-cow-placement-historical-lb-2531, downloaded
8 September16:12 UTC. Source was extracted from a literal SOURCE assignment;
no downloaded notebook cells were executed. Hashes/definitions:EXTRACTION.json.

The complete inherited agent is Boatlee V29-R1 version346682136 (Apache2.0),
whose farm tape is attributed to RngRng. We already have a C++ Boatlee V29 port.
This notebook adds a carried-animal placement repair and three tail-sale
parameters. The title's2531 rating refers to old submission55964457, not a new
validated score for this notebook. Upstream reports77 wins/68 losses/1tie in
146 later ladder games; their opponent cohorts were not matched.

Useful bounded repair: when the scheduled PLACE targets empty soil and the
worker actually carries the animal, BUILD its structure now and PLACE on the
next originally idle slot. Refuse if the next slot is busy or the animal is
missing. The next observation must confirm an empty compatible structure and
retained carried animal. The published version has no day-boundary guard;
any borrowed improvement must add same-day/remaining-turn/worker continuity.

This does not repair an unaffordable animal purchase: actual ownership is a
prerequisite. Mixed day10 currently has a failed placement/feed at tile14; first
trace actual acquisition and carried inventory. Do not transplant a build
repair until that cause is established. Other inherited trade guards reserve
scheduled pickups/sales before extra sales. The notebook also documents an
extra-sale-budget accounting issue when the ten-order list is already full.

Decision: retain this component as an attributed transaction-completion idea.
A new whole-agent port is lower priority than existing V25 execution ablations
and the stronger current farm. No claim of local parity or competitive gain.
