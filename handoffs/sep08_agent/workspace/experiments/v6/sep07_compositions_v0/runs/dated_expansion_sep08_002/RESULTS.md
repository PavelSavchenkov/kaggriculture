# Dated expansion search results

375 proposals,16 economic scenarios,33 selected farms; 544 full games including16 exact old controls. No strong-agent promotion.

Estimated-versus-realized margin rank correlations: {'public_router': 0.9739304812834224, 'observed_sale_lead_start_216': 0.9649064171122995}. These are discovery results with fixed-flow estimates and live opponents.

## Against public_router

| Agent | Family | Day/count/hands | Cash | Margin | Win utility | Estimated min cash |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| dated_expansion_p356 | mixed_berries | 6/12/12 | 54197.00 | -52146.25 | 0.000 | -9741.12 |
| dated_expansion_p355 | mixed_berries | 6/12/10 | 58459.00 | -52824.75 | 0.000 | -9508.12 |
| dated_expansion_p362 | mixed_berries | 8/12/12 | 54426.00 | -53217.25 | 0.000 | -6984.50 |
| dated_expansion_p361 | mixed_berries | 8/12/10 | 57625.25 | -54773.75 | 0.000 | -6751.50 |
| dated_expansion_p228 | mixed | 10/12/8 | 54104.00 | -63105.50 | 0.000 | -446.75 |
| dated_expansion_p364 | mixed_berries | 10/6/10 | 56051.00 | -63199.00 | 0.000 | -446.75 |
| dated_expansion_p229 | mixed | 10/12/10 | 51900.75 | -63289.00 | 0.000 | -446.75 |
| dated_expansion_p365 | mixed_berries | 10/6/12 | 52093.50 | -64061.25 | 0.000 | -446.75 |

## Against observed_sale_lead_start_216

| Agent | Family | Day/count/hands | Cash | Margin | Win utility | Estimated min cash |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| dated_expansion_p362 | mixed_berries | 8/12/12 | 57130.50 | -57133.00 | 0.000 | -6984.50 |
| dated_expansion_p356 | mixed_berries | 6/12/12 | 55441.25 | -57795.00 | 0.000 | -9741.12 |
| dated_expansion_p361 | mixed_berries | 8/12/10 | 60795.25 | -57963.25 | 0.000 | -6751.50 |
| dated_expansion_p355 | mixed_berries | 6/12/10 | 58758.12 | -58556.38 | 0.000 | -9508.12 |
| dated_expansion_p229 | mixed | 10/12/10 | 52332.25 | -67393.50 | 0.000 | -446.75 |
| dated_expansion_p365 | mixed_berries | 10/6/12 | 55061.00 | -67395.50 | 0.000 | -446.75 |
| dated_expansion_p364 | mixed_berries | 10/6/10 | 58535.00 | -67875.75 | 0.000 | -446.75 |
| dated_expansion_p91 | cow | 8/6/10 | 46681.50 | -68114.00 | 0.000 | -446.75 |

The restricted funding selection does not beat the retained unconstrained leaders. Smaller early funding gaps alone do not solve dense-farm service losses. Use actual-birth/service decomposition and exact day scheduling on the larger promising farms; improve economic forecasting without treating the restriction as a hard optimality rule.


Animal output decomposition for the larger early mixed farms is in ../dated_expansion_sep08_001/ANIMAL_GAPS.json. It uses exact biology on actual birth/survival dates, then actual feed/care/collection masks, then real harvested output. Most remaining loss after delayed births is service; harvest/model loss is small except some goose eggs. These modes diagnose the bottleneck without assuming arbitrary full service is optimal.
