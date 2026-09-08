# animal_groups_sheep12

Experimental strongest-parent animal-course policy. Mode 1, forced family
1, forced choice 2; -1 means observed-state economic selection.
Mode 0 is the unchanged parent; mode 1 uses compiled market orders; mode 2
adds live inventory-based sale anticipation and removes empty sale orders.

Choose crops versus one/three cows on day 15 in the imported wool farm, or
crops versus sheep on three eligible tiles on day 20 in the crop farm. Preserve
the observed strawberry branch and select the cow weed schedule from current
farm state. Price full courses across 32 conditional future shop scenarios,
charging compiled labor and other fixed costs. Prior general animal selection
and other parent behavior remain active before a new course is selected.

After investment, a failed daily guard records a diagnostic and continues the
closest complete course instead of reverting to the old herd. This is not yet
validated as a general repair. Static tables are immutable; all decisions and
diagnostics are per instance and reset. Only public/own observations are used.
Original animal compiler and donor lineage are in LIBRARY_LINEAGE.json and
the referenced parent run. Required operations and league validation pending.
