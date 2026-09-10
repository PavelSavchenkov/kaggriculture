# History timing with unchanged ending market inventory

Experimental variant of ../history_course, retaining its external Atakan fixed farm course, compiler and purchase repair. Default course 0. It additionally rejects a one-turn delay whose two-turn projection changes ending shared market inventory. This addresses a traced price-floor effect: floor sales add no inventory, so a locally profitable delay can lower later sale prices even when both farm states match.

The original history_course remains the separate control. This variant has no promotion or submission claim until its own tests complete. Provenance, input restrictions and runtime assumptions are those in ../history_course/README.md; root agent sources are unchanged.
