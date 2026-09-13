# Exact schedules within the requested time budget

Every row, physical input and binary hash is checked. Original outputs, trade slots and retained state remain exact. Each cell is a measured cold policy call at the stated workforce. Internal retries share that call's budget; results from separate calls are never combined.

| Profile | Limit | Original | +1 | +2 |
|---|---:|---:|---:|---:|
| deferred500 | 500ms | 89/145 (61.4%); 265ms | 109/145 (75.2%); 250ms | 102/145 (70.3%); 262ms |
| raw90_500 | 500ms | 91/145 (62.8%); 264ms | 113/145 (77.9%); 247ms | 108/145 (74.5%); 259ms |
| main_first500 | 500ms | 86/145 (59.3%); 302ms | 103/145 (71.0%); 293ms | 98/145 (67.6%); 303ms |

Cells show within-budget exact success and mean runtime of all calls, including failures. The solver has a soft limit; any successful result after the requested limit is excluded from these counts and retained separately in the JSON.
