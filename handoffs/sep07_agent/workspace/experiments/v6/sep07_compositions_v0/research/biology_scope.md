# First biological estimator component

`Cohort` represents counted individual lifecycles, with zero-based inclusive
start and exclusive end days. Replay-derived cohorts retain individual tile and
turn boundaries in compositions.json. A long maintained wheat allocation must
expand into repeated crop lifecycles; it must not be interpreted as one wheat
plant surviving twenty days. A higher-level maintained-count proposal should
support both repeated planting and explicit individual lifetimes.

Given explicit day masks, biology.hpp calculates crop growth, animal care banks,
input wheat/fertilizer, output and successful field operations. Inputs are
assumed available, harvested produce is assumed collectable, and work is
assumed schedulable. This is not a profit estimate or a feasibility proof.
The default services productive crops and animals; day masks support local
exceptions without increasing the detail of every evaluation.

Default crop harvest is the last requested productive day for one-shot crops,
and daily collection for ongoing crops. Optional fertilizer is renewed every
three days starting when it can affect yield. Its economic value is not yet
judged. Animals receive full feed/care and fertilizer collection; terminal and
price-driven service exceptions belong to later local optimization.

Tests compare full per-day output and accepted service/input counts against
128 exact games across all eight crop/animal types and combinations of missed
watering, feed, care and harvest. These isolate biology by supplying inputs at
the start of each day. They do not validate worker travel, storage, cash, market
timing or disposal. No constant cash charge is added for building structures:
the official engine charges only the worker action.

Known boundary: the engine permits harvesting during gradual decay after the
nominal lifespan. The first component ends its productive-service model at that
boundary. Exact hour-zero salvage and later decay-day harvesting are potentially
useful extensions, not impossible strategies. Ongoing-crop end-day timing also
needs exact realization when choosing delayed sales/harvests.

144 profiles from 72 current top-player compositions, repeated 100 times,
averaged 3.86 microseconds/profile on this CPU. This speed supports trying cheap
economic evaluation; ranking accuracy remains unmeasured. Fertilization profiles
show a large strawberry-output difference, which matches its biological value,
but buying every fertilizer input would have substantial cost. Resource reuse,
labor, placement and price impact must be added before ranking compositions.
