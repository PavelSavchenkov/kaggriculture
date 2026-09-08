# Wheat bundle comparison

Reject a universal smaller wheat bundle. It helps the small mixed farm but hurts the larger cold and dense source farms on broad cash/margin measures. Delivery needs a complete route and resource assignment, not only smaller loads.

480 profiled games; 128 old complete controls exact.

| Case | Opponent | Bundle | Cash gain | Margin gain | Fed fraction | Moves |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| mixed | public_router | 1 | +442.38 | +665.25 | 0.9594 | 1743.75 |
| mixed | public_router | 2 | +305.62 | +620.75 | 0.9646 | 1786.62 |
| mixed | public_router | 4 | +0.00 | +0.00 | 0.9625 | 1835.50 |
| mixed | observed_sale_lead_start_216 | 1 | +362.50 | +469.38 | 0.9594 | 1742.25 |
| mixed | observed_sale_lead_start_216 | 2 | +193.75 | +167.38 | 0.9656 | 1786.25 |
| mixed | observed_sale_lead_start_216 | 4 | +0.00 | +0.00 | 0.9646 | 1833.25 |
| p355 | public_router | 1 | -5813.75 | -10408.38 | 0.7795 | 3560.38 |
| p355 | public_router | 2 | -3387.12 | -4434.75 | 0.8133 | 3413.75 |
| p355 | public_router | 4 | +0.00 | +0.00 | 0.8037 | 3521.00 |
| p355 | observed_sale_lead_start_216 | 1 | -5826.44 | -10007.31 | 0.7755 | 3572.62 |
| p355 | observed_sale_lead_start_216 | 2 | -1631.56 | -1331.06 | 0.8201 | 3482.75 |
| p355 | observed_sale_lead_start_216 | 4 | +0.00 | +0.00 | 0.7957 | 3550.75 |
| p362 | public_router | 1 | -1825.62 | -3277.12 | 0.8171 | 3962.12 |
| p362 | public_router | 2 | -991.00 | +246.00 | 0.8487 | 3810.00 |
| p362 | public_router | 4 | +0.00 | +0.00 | 0.8505 | 3988.50 |
| p362 | observed_sale_lead_start_216 | 1 | -1793.25 | -4175.00 | 0.7987 | 3893.12 |
| p362 | observed_sale_lead_start_216 | 2 | -742.88 | -1055.75 | 0.8354 | 3778.75 |
| p362 | observed_sale_lead_start_216 | 4 | +0.00 | +0.00 | 0.8490 | 3858.38 |
| p4 | public_router | 1 | -2154.06 | -3546.44 | 0.6096 | 4349.50 |
| p4 | public_router | 2 | +22.62 | -324.56 | 0.6167 | 4339.12 |
| p4 | public_router | 4 | +0.00 | +0.00 | 0.6198 | 4344.88 |
| p4 | observed_sale_lead_start_216 | 1 | -2630.44 | -5337.31 | 0.6131 | 4361.25 |
| p4 | observed_sale_lead_start_216 | 2 | -368.25 | -2235.38 | 0.6166 | 4352.88 |
| p4 | observed_sale_lead_start_216 | 4 | +0.00 | +0.00 | 0.6206 | 4349.50 |
| p55 | public_router | 1 | -3848.50 | -10287.94 | 0.7253 | 3870.81 |
| p55 | public_router | 2 | -1994.38 | -4639.56 | 0.7452 | 3952.56 |
| p55 | public_router | 4 | +0.00 | +0.00 | 0.7388 | 3982.44 |
| p55 | observed_sale_lead_start_216 | 1 | -3378.25 | -8752.88 | 0.7129 | 3871.25 |
| p55 | observed_sale_lead_start_216 | 2 | -1983.25 | -3709.62 | 0.7445 | 3958.00 |
| p55 | observed_sale_lead_start_216 | 4 | +0.00 | +0.00 | 0.7419 | 3985.00 |

Use the exact day-solver delivery witness to design a reusable day-task/unit-route planner with current stocks and optional service tasks. Separate physical action planning from market projection so new schedules retain valid sales/purchases. Require old behavior parity after refactoring.
