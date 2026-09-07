# TokenJunkieLabs Kaggriculture farm agent

A standalone Python agent that manages livestock, crops, workers, land purchases and sales. It estimates future prices from visible production and town demand, assigns workers to nearby tasks, and brings goods home before the season ends. The current policy caps livestock at 28 and daily hired hands at 10.

The agent uses only the standard library and requires no trained model, API key or network access during a game. The code cell retrieves a fixed release and creates submission.tar.gz with main.py at its root and the license notices included. Save & Run All, then select the archive under Output -> Submit to Competition.

## Measured results

On the unmodified official game interpreter, this version won 48/48 matches across six seeds, both player positions and four baseline policies. On five separately reserved seeds, both positions, it beat the previous version 10/10 but beat the compact-22 policy only 4/10. These comparisons do not establish leaderboard strength against other entrants.

[Evaluation and reports](https://github.com/woahwhattheheck/commons/actions/runs/34083400748) | [Reserved-seed comparison](https://github.com/woahwhattheheck/commons/actions/runs/34083400964)

## Source and license

Copyright 2026 Bryce Xavier Muhlnickel / TokenJunkieLabs. Owner-authored material is offered under MIT OR CC-BY-4.0, at your choice. This notebook packages the agent without modifying it. Upstream material retains its own license.

[Source, license texts and attribution](https://github.com/woahwhattheheck/commons/tree/2dc9d9f955adb2f8ddbf238e6e5a334004178a9a/revenue/kaggriculture/20260907-offline-agent).
