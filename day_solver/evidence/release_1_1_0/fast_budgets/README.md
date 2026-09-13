# Exact schedules within the requested time budget

Every row, physical input and binary hash is checked. Original outputs, trade slots and retained state remain exact. Each cell is a measured cold policy call at the stated workforce. Internal retries share that call's budget; results from separate calls are never combined.

| Profile | Limit | Original | +1 | +2 |
|---|---:|---:|---:|---:|
| deferred200 | 200ms | 49/145 (33.8%); 152ms | 58/145 (40.0%); 146ms | 57/145 (39.3%); 148ms |
| raw90_200 | 200ms | 53/145 (36.6%); 151ms | 58/145 (40.0%); 148ms | 57/145 (39.3%); 150ms |
| deferred2 | 2000ms | 115/145 (79.3%); 424ms | 133/145 (91.7%); 420ms | 135/145 (93.1%); 422ms |
| raw90_2 | 2000ms | 115/145 (79.3%); 422ms | 132/145 (91.0%); 420ms | 135/145 (93.1%); 420ms |

Cells show within-budget exact success and mean runtime of all calls, including failures. The solver has a soft limit; any successful result after the requested limit is excluded from these counts and retained separately in the JSON.
