# One-turn Otter delays on mixed market turns

Allowing the same rule on purchase/hiring turns increases guarded Otter margin gains to $78, $31 and $177 over original orders in the same three exposed episodes. Own cash gains are $78/$31/$119; rival cash changes are $0/$0/-$58. There are 50 guarded decisions instead of five under the sale-only restriction. No actual future opponent orders enter the decision.

The only rule change is eligibility for mixed turns, with a new check that current workers, hires and land match the original projection. Existing non-sale order slots are kept. Two-turn stock, seed, worker, land, carried input, missing-input and discard checks remain. It still waits only one turn across known demand, at most one product per turn, excluding products visibly ready on the rival farm now or during the last four turns.

Full-engine replay agrees exactly with financial cash and decisions for all 18 games (both seats, original and two variants). Both farms’ physical work state matches the original after every turn. Final stock, production, discarded goods, failed actions, requested work and shops match. The unchanged sale-only mode also reproduces its previous six source-game results exactly, excluding runtime.

Without the rival-ready guard, margins are -$4/+$137/+$318. Those larger gains in two exposed games do not establish a safer or stronger rule; the next frozen test covers the other 138 recorded farm plans and retains all failures. No general promotion based on these three Otter games.

This confirms that observed immediate selling leaves some exploitable timing opportunities. It does not reveal Otter’s implementation rationale, the value of longer holding periods, or how its live agent would react.

Evidence: PROTOCOL.json, OTTER_RESULTS.json, engine.jsonl, engine_events.jsonl and financial.jsonl. Binary build/aeda4e3b47bbc4afa8d5/replay_timing. Follow-up: runs/wait_mixed_transfer_v0.
