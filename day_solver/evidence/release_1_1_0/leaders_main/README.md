# Exact schedules within the requested time budget

Every row, physical input and binary hash is checked. Original outputs, trade slots and retained state remain exact. Each cell is an independent cold call at the stated workforce; profiles and worker-count attempts are never combined into one time budget.

| Profile | Limit | Original | +1 | +2 | +3 |
|---|---:|---:|---:|---:|---:|
| baseline | 4000ms | 123/145 (84.8%); 1097ms | 141/145 (97.2%); 733ms | 139/145 (95.9%); 755ms | 140/145 (96.6%); 710ms |
| candidate | 4000ms | 123/145 (84.8%); 1096ms | 143/145 (98.6%); 723ms | 139/145 (95.9%); 757ms | 140/145 (96.6%); 712ms |

Cells show within-budget exact success and mean runtime of all calls, including failures. The solver has a soft limit; any successful result after the requested limit is excluded from these counts and retained separately in the JSON.
