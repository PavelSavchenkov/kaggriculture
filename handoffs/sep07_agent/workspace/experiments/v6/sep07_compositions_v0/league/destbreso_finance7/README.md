# destbreso_finance7

Typed C++ port of the public v7.38 Finance7 notebook. The native base has the
same source and schedule hashes as the teammate's three-day router. This port
uses the original base and destbreso's mirror and hire-financing additions.
Mode0 is bare, mode1 mirror only, mode2 finance only, mode3 both. Episode state
belongs to each instance; shared native action tables are immutable.

Mirror detection uses twenty matching public farm signatures in turns100–143.
It appends the identified course's next non-wheat sales. Hire financing prepends
a sale from observed stock when cash cannot cover requested hires and an order
slot is free. This source heuristic does not model current worker withdrawals
or exact simultaneous prices. All four modes match 8,628 original actions each, including active layer cases.
Generic/debug/thread, PASS and self-play checks pass. In256games/opponent
it wins151against Junghoon78 but7against public_router,2teammate and0v4.
Retained league opponent; no incumbent promotion. The mirror-only component
beats its bare base in32/32discovery games; finance alone slightly regresses.
Exact source hashes, adaptations and attribution are in IMPORT.json and NOTICE.
