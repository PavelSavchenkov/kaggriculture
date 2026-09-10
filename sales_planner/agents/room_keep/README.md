# Day-end storage rule

Wraps persistent `early_structure_cow`. Removes guaranteed ineffective orders on sale-only turns, as `compact_sales` does. Before automatic day-end deposits, it may append a small sale if the exact refill leaves every shed item at least as numerous as without that sale. Keeps the parent's worker actions.

The projection receives the legal own observation and current requested worker actions. It creates a local simulator with an arbitrary seed, disables orders and day-end processing, and returns only own inventories. No hidden rival stock, live simulator, seed or future shops are available. Candidate ranking uses current public quotes; final margin still needs live opponent validation.

Same parent provenance and reuse restrictions as the persistent root agent. This is an experimental integration, not a submitted agent. Local per-instance state resets every game. Current board support is the default 10 × 10; one-turn days are unsupported by the projection and keep the parent.
