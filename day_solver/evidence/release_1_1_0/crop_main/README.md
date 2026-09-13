# Exact schedules within the requested time budget

Every row, physical input and binary hash is checked. Original outputs, trade slots and retained state remain exact. Each cell is a measured cold policy call at the stated workforce. Internal retries share that call's budget; results from separate calls are never combined.

| Profile | Limit | Original |
|---|---:|---:|
| baseline | 4000ms | 1921/1943 (98.9%); 311ms |
| candidate | 4000ms | 1922/1943 (98.9%); 307ms |

Cells show within-budget exact success and mean runtime of all calls, including failures. The solver has a soft limit; any successful result after the requested limit is excluded from these counts and retained separately in the JSON.
