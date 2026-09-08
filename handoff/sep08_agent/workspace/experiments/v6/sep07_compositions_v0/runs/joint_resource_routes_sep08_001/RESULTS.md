# Cumulative-input route results

Do not replace the compiler with these routes. Cumulative cargo helps some dense cases but does not recover the losses from replacing the original split compiler. The scarcity penalty is not generally useful.

960 full games; 192 complete old route controls equal. No strongest-agent promotion.

| Farm | Opponent | Mode | Cash gain vs joint | Cash gain vs original split | Margin gain vs joint |
| --- | --- | ---: | ---: | ---: | ---: |
| mixed | public_router | 1 | -206.25 | -85.75 | -436.38 |
| mixed | public_router | 2 | +3.38 | +123.88 | +8.50 |
| mixed | public_router | 3 | +33.62 | +154.12 | +35.00 |
| mixed | public_router | 7 | -2987.50 | -2867.00 | -5313.25 |
| mixed | observed_sale_lead_start_216 | 1 | -294.62 | -81.75 | -529.38 |
| mixed | observed_sale_lead_start_216 | 2 | +37.25 | +250.12 | +76.38 |
| mixed | observed_sale_lead_start_216 | 3 | +21.00 | +233.88 | +16.50 |
| mixed | observed_sale_lead_start_216 | 7 | -3231.00 | -3018.12 | -5871.50 |
| goose | public_router | 1 | +1.00 | -3141.62 | +1.75 |
| goose | public_router | 2 | +0.00 | -3142.62 | +0.00 |
| goose | public_router | 3 | +1.00 | -3141.62 | +1.75 |
| goose | public_router | 7 | -716.88 | -3859.50 | -816.50 |
| goose | observed_sale_lead_start_216 | 1 | +0.12 | -3060.88 | +1.62 |
| goose | observed_sale_lead_start_216 | 2 | +0.00 | -3061.00 | +0.00 |
| goose | observed_sale_lead_start_216 | 3 | +0.12 | -3060.88 | +1.62 |
| goose | observed_sale_lead_start_216 | 7 | -720.38 | -3781.38 | -855.62 |
| p355 | public_router | 1 | +77.50 | -4899.88 | +996.75 |
| p355 | public_router | 2 | -22.88 | -5000.25 | +480.50 |
| p355 | public_router | 3 | +115.00 | -4862.38 | +2857.00 |
| p355 | public_router | 7 | -961.88 | -5939.25 | -1368.50 |
| p355 | observed_sale_lead_start_216 | 1 | +432.50 | -4873.44 | +2994.62 |
| p355 | observed_sale_lead_start_216 | 2 | +508.25 | -4797.69 | +2645.12 |
| p355 | observed_sale_lead_start_216 | 3 | +101.38 | -5204.56 | +3215.88 |
| p355 | observed_sale_lead_start_216 | 7 | -1536.12 | -6842.06 | -651.75 |
| p362 | public_router | 1 | -258.62 | -3653.25 | -905.62 |
| p362 | public_router | 2 | +234.12 | -3160.50 | +1502.88 |
| p362 | public_router | 3 | +275.75 | -3118.88 | +263.88 |
| p362 | public_router | 7 | +958.75 | -2435.88 | -2316.50 |
| p362 | observed_sale_lead_start_216 | 1 | +114.75 | -3832.75 | -18.88 |
| p362 | observed_sale_lead_start_216 | 2 | +274.50 | -3673.00 | +1167.38 |
| p362 | observed_sale_lead_start_216 | 3 | +1085.88 | -2861.62 | +1653.00 |
| p362 | observed_sale_lead_start_216 | 7 | +112.12 | -3835.38 | -3792.38 |
| p4 | public_router | 1 | +370.75 | -13884.00 | +3501.00 |
| p4 | public_router | 2 | +106.81 | -14147.94 | +80.50 |
| p4 | public_router | 3 | +1330.94 | -12923.81 | +4441.19 |
| p4 | public_router | 7 | +803.56 | -13451.19 | +2594.19 |
| p4 | observed_sale_lead_start_216 | 1 | +1413.50 | -13758.38 | +4464.38 |
| p4 | observed_sale_lead_start_216 | 2 | -1682.50 | -16854.38 | -3709.38 |
| p4 | observed_sale_lead_start_216 | 3 | +2723.25 | -12448.62 | +6642.25 |
| p4 | observed_sale_lead_start_216 | 7 | +711.25 | -14460.62 | +1082.12 |
| p55 | public_router | 1 | -486.56 | -5068.56 | -843.50 |
| p55 | public_router | 2 | -1037.69 | -5619.69 | -969.94 |
| p55 | public_router | 3 | -1008.31 | -5590.31 | -1048.75 |
| p55 | public_router | 7 | -2206.19 | -6788.19 | -5056.88 |
| p55 | observed_sale_lead_start_216 | 1 | -50.31 | -5075.44 | -1571.94 |
| p55 | observed_sale_lead_start_216 | 2 | -577.31 | -5602.44 | -543.19 |
| p55 | observed_sale_lead_start_216 | 3 | -108.25 | -5133.38 | -1122.19 |
| p55 | observed_sale_lead_start_216 | 7 | -2799.88 | -7825.00 | -5299.75 |

Mode bits: 1 costs initial inputs after crediting earlier route harvest/collection; 2 uses that calculation for withdrawals; 4 penalizes currently missing stock. Mode 7 combines all three.

Use exact day-solver schedules on changed midseason states to measure missing task dependencies and input transfers. Do not keep tuning route costs while leaving those constraints absent.
