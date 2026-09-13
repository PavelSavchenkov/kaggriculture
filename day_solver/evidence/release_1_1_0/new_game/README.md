# Exact schedules within the requested time budget

Every row, physical input and binary hash is checked. Original outputs, trade slots and retained state remain exact. Each cell is a measured cold policy call at the stated workforce. Internal retries share that call's budget; results from separate calls are never combined.

| Profile | Limit | Original | +1 | +2 | +3 |
|---|---:|---:|---:|---:|---:|
| main4 | 4000ms | 29/29 (100.0%); 390ms | 29/29 (100.0%); 392ms | 29/29 (100.0%); 393ms | 29/29 (100.0%); 386ms |
| main12 | 12000ms | 29/29 (100.0%); 431ms | 29/29 (100.0%); 395ms | 29/29 (100.0%); 390ms | 29/29 (100.0%); 381ms |
| deferred500 | 500ms | 24/29 (82.8%); 206ms | 26/29 (89.7%); 190ms | 24/29 (82.8%); 223ms | 24/29 (82.8%); 226ms |
| raw90_500 | 500ms | 24/29 (82.8%); 203ms | 26/29 (89.7%); 188ms | 25/29 (86.2%); 216ms | 25/29 (86.2%); 220ms |
| polish50_500 | 500ms | 25/29 (86.2%); 199ms | 26/29 (89.7%); 188ms | 25/29 (86.2%); 208ms | 25/29 (86.2%); 213ms |

Cells show within-budget exact success and mean runtime of all calls, including failures. The solver has a soft limit; any successful result after the requested limit is excluded from these counts and retained separately in the JSON.
