# Dated expansion search results

375 proposals,16 economic scenarios,34 selected farms; 560 full games including16 exact old controls. No strong-agent promotion.

Estimated-versus-realized margin rank correlations: {'public_router': 0.9110771581359817, 'observed_sale_lead_start_216': 0.8957983193277311}. These are discovery results with fixed-flow estimates and live opponents.

## Against public_router

| Agent | Family | Day/count/hands | Cash | Margin | Win utility | Estimated min cash |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| dated_expansion_p356 | mixed_berries | 6/12/12 | 54197.00 | -52146.25 | 0.000 | -9741.12 |
| dated_expansion_p355 | mixed_berries | 6/12/10 | 58459.00 | -52824.75 | 0.000 | -9508.12 |
| dated_expansion_p362 | mixed_berries | 8/12/12 | 54426.00 | -53217.25 | 0.000 | -6984.50 |
| dated_expansion_p352 | mixed_berries | 6/6/10 | 57269.75 | -53435.50 | 0.000 | -6719.75 |
| dated_expansion_p353 | mixed_berries | 6/6/12 | 54083.50 | -54042.00 | 0.000 | -6952.75 |
| dated_expansion_p361 | mixed_berries | 8/12/10 | 57625.25 | -54773.75 | 0.000 | -6751.50 |
| dated_expansion_p193 | mixed | 4/12/10 | 52564.75 | -58103.75 | 0.000 | -6549.75 |
| dated_expansion_p358 | mixed_berries | 8/6/10 | 55690.00 | -59113.75 | 0.000 | -3942.25 |

## Against observed_sale_lead_start_216

| Agent | Family | Day/count/hands | Cash | Margin | Win utility | Estimated min cash |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| dated_expansion_p362 | mixed_berries | 8/12/12 | 57130.50 | -57133.00 | 0.000 | -6984.50 |
| dated_expansion_p356 | mixed_berries | 6/12/12 | 55441.25 | -57795.00 | 0.000 | -9741.12 |
| dated_expansion_p361 | mixed_berries | 8/12/10 | 60795.25 | -57963.25 | 0.000 | -6751.50 |
| dated_expansion_p355 | mixed_berries | 6/12/10 | 58758.12 | -58556.38 | 0.000 | -9508.12 |
| dated_expansion_p352 | mixed_berries | 6/6/10 | 58994.50 | -60000.50 | 0.000 | -6719.75 |
| dated_expansion_p353 | mixed_berries | 6/6/12 | 55017.50 | -61045.25 | 0.000 | -6952.75 |
| dated_expansion_p193 | mixed | 4/12/10 | 52361.75 | -62390.50 | 0.000 | -6549.75 |
| dated_expansion_p192 | mixed | 4/12/8 | 54800.25 | -64278.25 | 0.000 | -6371.75 |

Compare forecasts and actual birth dates/output. The highest estimates request large negative early balances; preserve this as an error and add a separate selection constrained to no extra funding deficit above the unchanged opening. Do not infer feasibility from an optimistic biological projection.


Animal output decomposition for the larger early mixed farms is in ../dated_expansion_sep08_001/ANIMAL_GAPS.json. It uses exact biology on actual birth/survival dates, then actual feed/care/collection masks, then real harvested output. Most remaining loss after delayed births is service; harvest/model loss is small except some goose eggs. These modes diagnose the bottleneck without assuming arbitrary full service is optimal.
