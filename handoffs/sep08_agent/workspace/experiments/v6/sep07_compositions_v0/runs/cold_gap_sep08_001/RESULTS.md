# Cold-farm composition, placement and service gap

The original p362 output and212-turn work deficit reproduce exactly. This diagnosis uses1024 saved profiles and512 reversed full games; every reversed cash/action hash/profile matches the original game.

Against empty_sale_slots_m2: cold cash 62433.40, opponent 122928.89; hire costs 8035.12 versus 4038.54.

| Product | Intended standard | Intended fertilized | Actual lifetimes, standard service | Realized cold output | Realized opponent output |
| --- | ---: | ---: | ---: | ---: | ---: |
| WHEAT | 140 | 210 | 128.00 | 127.77 | 517.21 |
| CARROT | 0 | 0 | 0.00 | 0.00 | 74.16 |
| TOMATO | 0 | 0 | 0.00 | 0.00 | 3.19 |
| STRAWBERRY | 192 | 192 | 191.88 | 189.84 | 247.55 |
| MELON | 72 | 72 | 72.00 | 72.00 | 72.00 |
| EGG | 152 | 152 | 143.78 | 116.24 | 61.00 |
| MILK | 168 | 168 | 163.50 | 150.52 | 236.05 |
| WOOL | 172 | 172 | 152.59 | 134.55 | 185.39 |
| FERTILIZER | 368 | 368 | 347.45 | 335.64 | 379.16 |

Conditional intended-plan economic forecasts on the same observed rival flows:
- Mode 0: cash 64472.26, margin -47096.89, minimum cash -6957.06; funding deficits in 256/256, missing inputs in 0/256.
- Mode 1: cash 64389.24, margin -49988.40, minimum cash -10091.82; funding deficits in 256/256, missing inputs in 0/256.

Against public_router: cold cash 61321.60, opponent 119001.78; hire costs 8036.24 versus 5618.66.

| Product | Intended standard | Intended fertilized | Actual lifetimes, standard service | Realized cold output | Realized opponent output |
| --- | ---: | ---: | ---: | ---: | ---: |
| WHEAT | 140 | 210 | 128.00 | 127.89 | 537.38 |
| CARROT | 0 | 0 | 0.00 | 0.00 | 75.93 |
| TOMATO | 0 | 0 | 0.00 | 0.00 | 0.00 |
| STRAWBERRY | 192 | 192 | 192.00 | 190.48 | 261.81 |
| MELON | 72 | 72 | 72.00 | 72.00 | 72.00 |
| EGG | 152 | 152 | 145.97 | 117.93 | 53.70 |
| MILK | 168 | 168 | 165.43 | 153.27 | 250.32 |
| WOOL | 172 | 172 | 155.16 | 141.15 | 151.12 |
| FERTILIZER | 368 | 368 | 350.02 | 338.59 | 374.30 |

Conditional intended-plan economic forecasts on the same observed rival flows:
- Mode 0: cash 63537.37, margin -44053.70, minimum cash -6955.21; funding deficits in 256/256, missing inputs in 0/256.
- Mode 1: cash 63579.30, margin -46610.68, minimum cash -10079.80; funding deficits in 256/256, missing inputs in 0/256.

Interpretation limits:
- All comparisons use the same frozen seed2300000..2300127 games/both seats. Reversed runs reproduce both action hashes, cash and profiles exactly.
- Standard service uses fertilizer on ongoing crops only, productive one-shot harvest, and daily animal feed/care/collect/harvest. All-crops-fertilized changes one-shot service too.
- Observed lifetimes retain actual births and end dates, including losses/delays. Their ideal service is a conditional dated model, not a feasible schedule or certified bound.
- Observed animal service retains recorded successful feed/care/collect day masks but assumes daily harvest; it cannot reconstruct exact intra-day harvest/cap losses.
- Financial forecasts retain fixed opponent flows from these games, original intended support, heuristic deposit times, approximate order interleaving and visible funding/input deficits. They do not predict opponent policy responses or demonstrate feasible cash.
