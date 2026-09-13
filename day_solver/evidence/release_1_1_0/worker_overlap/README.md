# Exact schedules within the requested time budget

Every row, physical input and binary hash is checked. Original outputs, trade slots and retained state remain exact. Each cell is a measured cold policy call at the stated workforce. Internal retries share that call's budget; results from separate calls are never combined.

| Profile | Limit | -1 | Original |
|---|---:|---:|---:|
| deferred500 | 500ms | 62/145 (42.8%); 271ms | 89/145 (61.4%); 266ms |
| raw90_500 | 500ms | 62/145 (42.8%); 271ms | 92/145 (63.4%); 265ms |

Cells show within-budget exact success and mean runtime of all calls, including failures. The solver has a soft limit; any successful result after the requested limit is excluded from these counts and retained separately in the JSON.
