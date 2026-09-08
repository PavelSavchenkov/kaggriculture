# Joint crop/animal routes

Reject this joint-route replacement as a general compiler improvement. Small mixed farm improves slightly, but dense and goose farms lose. Inspect blocked input tasks and dependent crop work before another route rule.

576 full games; 192 complete old compiler controls exact. No strongest-agent promotion.

| Farm | Opponent | Start day | Cash gain | Margin gain |
| --- | --- | ---: | ---: | ---: |
| mixed | public_router | 14 | +120.50 | +721.62 |
| mixed | public_router | 0 | +318.12 | +504.50 |
| mixed | observed_sale_lead_start_216 | 14 | +212.88 | +799.38 |
| mixed | observed_sale_lead_start_216 | 0 | +334.62 | +519.75 |
| goose | public_router | 14 | -3142.62 | -3458.75 |
| goose | public_router | 0 | -5465.12 | -6359.38 |
| goose | observed_sale_lead_start_216 | 14 | -3061.00 | -3345.38 |
| goose | observed_sale_lead_start_216 | 0 | -5279.88 | -6200.50 |
| p355 | public_router | 14 | -4977.38 | -12038.12 |
| p355 | public_router | 0 | -10057.12 | -18093.88 |
| p355 | observed_sale_lead_start_216 | 14 | -5305.94 | -11715.06 |
| p355 | observed_sale_lead_start_216 | 0 | -9804.69 | -17173.12 |
| p362 | public_router | 14 | -3394.62 | -6794.00 |
| p362 | public_router | 0 | -4470.00 | -13131.62 |
| p362 | observed_sale_lead_start_216 | 14 | -3947.50 | -8357.75 |
| p362 | observed_sale_lead_start_216 | 0 | -4742.00 | -13005.62 |
| p4 | public_router | 14 | -14254.75 | -26160.69 |
| p4 | public_router | 0 | -29869.88 | -57981.50 |
| p4 | observed_sale_lead_start_216 | 14 | -15171.88 | -27573.50 |
| p4 | observed_sale_lead_start_216 | 0 | -30436.62 | -62387.75 |
| p55 | public_router | 14 | -4582.00 | -9625.44 |
| p55 | public_router | 0 | -26855.00 | -50469.56 |
| p55 | observed_sale_lead_start_216 | 14 | -5025.12 | -9191.25 |
| p55 | observed_sale_lead_start_216 | 0 | -25914.12 | -47987.94 |

Mode1 starts day14 and retains earlier behavior. On p355 versus public, eggs+11 and wool+13.75 accompany strawberries-49.25 and milk-13.875; hires unchanged. More animal service can displace crop work. These aggregate changes do not yet prove the specific internal scheduling cause.

Trace the first changed mixed day and count unfinished/blocked tasks, current inputs and idle workers. Compare with a day-solver feasible schedule on exactly that start state. Incorporate dependent task work and availability into route assignment.
