# Data

MANIFEST.json records 297 exposed episodes and 25 reserved episodes, with source URLs, replay hashes, selected team/submission metadata and calendar versions. Repeated versions or different names do not prove independent agent families. The latest corrected extraction is the default; old version hashes remain available for historical audits.

A `.calendar` is the text `SPCAL1` format read by `include/case.hpp`. It contains both players' initial accounts, configuration, revealed-shop timeline, ordered resource events, requested market orders, worker-action witnesses and recorded states for checking the replay. Item, action, market-order and resource-event codes are defined in `scripts/extract_calendars.py` and the C++ headers.

This is **benchmark data**, not an observation object to pass wholesale into a live policy. The controller receives our supplied calendar/order plan and legal current information. Full rival action tapes, private resources and recorded outcomes belong to evaluation or explicitly supplied historical stress scenarios. In the public sale-only benchmark, accepted future own purchases are explicitly supplied plan quantities; they are not predictions made by the sales planner.

The small compressed fixtures are:

| Episode | Why keep it |
| --- | --- |
| 107273702 | A small carrot-sale gain can fund an extra seed purchase, reduce later feed purchases and change farming. |
| 107323879 | A milk-sale gain funds two otherwise failed, unused seed purchases. |
| 107342138 | A plan from the last frozen public timing confirmation. |

The first two explain the funded-purchase contract. All three are exposed regression cases, not a new holdout. `scripts/prepare_data.py` verifies hashes and restores them without the archive or network. It can restore other exposed episodes from the local archive or explicit Kaggle downloads. Generated data stays in ignored `data/cache/` and `replays/`.

The reserved episodes were acquired at the final 04:50 UTC refresh. Their action data was not inspected or extracted during this session or packaging. Default preparation and checks refuse them. Preserve that reservation until a new candidate and comparison protocol are frozen, then record their exposure before use.
