# Wheat pickup bundle experiment

Read RESULTS.md and ANALYSIS.json. One/two/four-item wheat pickups were compared on a small cold farm, two larger cold farms, and two dense replay-derived farms.480 profiled games,128 exact old controls and136 operational games pass. Smaller bundles are rejected as a general improvement; they help the small farm but usually reduce larger-farm performance. Core source and every candidate remain available with full local/source lineage.

The next step is joint route/input assignment and separating physical actions from market projection while preserving old behavior exactly. This study does not change the accepted strong agent. Run prepare.py,run_discovery.py,analyze.py,check_operations.py via conda run -n kaggriculture in a fresh output copy. Preparation/execution intentionally refuse to overwrite existing packages/results.
