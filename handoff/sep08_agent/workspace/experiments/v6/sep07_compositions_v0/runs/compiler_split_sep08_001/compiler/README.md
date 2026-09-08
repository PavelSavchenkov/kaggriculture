# Physical and market planning boundary

plan_units returns the old physical actions and the market context. plan_market projects whichever physical actions it receives, then emits the old market policy. Default act calls them consecutively. This refactor changes no strategy or parameters; exact controls are required before plugging in a different route planner. Context and all episode state are per instance/call.
