# Routing-score results

No general routing improvement. Current-tile bonus reduces moves but trades animal output for crops/fertilizer. Source55 gains cash, source4 and cold goose regress. No candidate is promoted. All 64 copied controls reproduce complete prior game records.

| Case | Mode | Opponent | Cash gain | Margin gain | Moves |
| --- | ---: | --- | ---: | ---: | ---: |
| mixed | 0 | public_router | 0.00 | 0.00 | 1611.81 |
| mixed | 1 | public_router | -19.38 | -70.12 | 1535.75 |
| mixed | 2 | public_router | 25.38 | 102.75 | 1600.19 |
| mixed | 3 | public_router | -14.12 | -95.75 | 1547.12 |
| mixed | 0 | observed_sale_lead_start_216 | 0.00 | 0.00 | 1610.06 |
| mixed | 1 | observed_sale_lead_start_216 | -201.62 | -462.38 | 1532.25 |
| mixed | 2 | observed_sale_lead_start_216 | 117.38 | 179.00 | 1599.81 |
| mixed | 3 | observed_sale_lead_start_216 | -193.25 | -480.62 | 1547.00 |
| goose | 0 | public_router | 0.00 | 0.00 | 1136.12 |
| goose | 1 | public_router | -250.38 | -252.88 | 1054.88 |
| goose | 2 | public_router | -75.62 | -70.00 | 1141.00 |
| goose | 3 | public_router | -242.75 | -263.12 | 1077.50 |
| goose | 0 | observed_sale_lead_start_216 | 0.00 | 0.00 | 1137.12 |
| goose | 1 | observed_sale_lead_start_216 | -242.12 | -254.62 | 1054.88 |
| goose | 2 | observed_sale_lead_start_216 | -81.25 | -86.38 | 1139.88 |
| goose | 3 | observed_sale_lead_start_216 | -235.50 | -263.75 | 1078.50 |
| p4 | 0 | public_router | 0.00 | 0.00 | 4339.06 |
| p4 | 1 | public_router | -2248.75 | -4374.50 | 3968.56 |
| p4 | 2 | public_router | -1342.38 | -1169.44 | 4372.06 |
| p4 | 3 | public_router | 90.62 | 1937.31 | 3969.06 |
| p4 | 0 | observed_sale_lead_start_216 | 0.00 | 0.00 | 4344.12 |
| p4 | 1 | observed_sale_lead_start_216 | -2089.62 | -7614.88 | 3975.75 |
| p4 | 2 | observed_sale_lead_start_216 | -1817.88 | -1611.62 | 4366.12 |
| p4 | 3 | observed_sale_lead_start_216 | -5.62 | 247.38 | 3979.75 |
| p55 | 0 | public_router | 0.00 | 0.00 | 3981.38 |
| p55 | 1 | public_router | 1971.75 | 287.00 | 3670.75 |
| p55 | 2 | public_router | -424.00 | -602.31 | 3939.56 |
| p55 | 3 | public_router | -122.12 | -1879.94 | 3698.44 |
| p55 | 0 | observed_sale_lead_start_216 | 0.00 | 0.00 | 3987.00 |
| p55 | 1 | observed_sale_lead_start_216 | 2352.50 | 696.12 | 3672.19 |
| p55 | 2 | observed_sale_lead_start_216 | -282.69 | 922.50 | 3934.75 |
| p55 | 3 | observed_sale_lead_start_216 | -282.62 | -2286.12 | 3684.31 |

Stop tuning greedy priority constants. Separate placement-induced early land spending and missing investment funding, then use complete daily task/routing constraints for remaining service losses.
