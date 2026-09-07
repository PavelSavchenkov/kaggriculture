# Four-random-shop dynamic robust agent

This observation-only C++ agent targets the normal 30-day game against PASS
with random weeds and only the first four shop unlocks enabled. It continuously
warms Bakery/Pet-native, Ice, Yarn, Pizza, and Smoothie continuations and selects
factorized routes from revealed shop identities. Handoffs occur only after the
required reveal, with topology-specific timing and observation-derived capacity
and terminal repairs.

The C0.44 seal was selected with
`J = 0.8 * mean(cash) + 0.2 * lower-CVaR10(cash)`. Its final paired audit spans
all `8^4 = 4,096` shop sequences, 32 seeds, and both seats. It scores mean cash
`152338.391411`, lower-CVaR10 `124478.600967`, and J `146766.433322`, a paired
`+2.075860` J over C0.43. It has zero failed actions and adds no discard; both
policies share two inherited one-item discards in that audit block.

The agent uses only `AgentInit` and ordinary `AgentObservation`. It assumes a
720-step episode, standard shop/product indices, four reveals at days 3/6/9/12,
and at most ten market orders. `reset` supports repeated independent episodes.
The sealed copy passes strict warning-as-error compilation, exact behavior
comparison with the experiment champion, and an ASan+UBSan complete-domain
replay over all 4,096 sequences and both seats.
