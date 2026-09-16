# Unrestricted day-policy measurements

Two sequential CPU14-pinned native Release runs; failed calls included. Late means days 20-28. Each primary row includes only days where the original schedule used no more hires than that row's cap.

| Cohort | Profile / cap | All days | Late days | Median / mean ms | Mean hires |
|---|---|---:|---:|---:|---:|
| dev | Balanced-4 cap 10 | 1,065/1,150 (92.6%) | 87/101 (86.1%) | 0.26 / 3.72 | 10.00 |
| dev | Balanced-4 cap 11 | 1,652/1,732 (95.4%) | 365/408 (89.5%) | 0.32 / 5.75 | 11.00 |
| dev | Balanced-4 cap 13 | 2,151/2,203 (97.6%) | 662/686 (96.5%) | 0.35 / 3.45 | 13.00 |
| dev | unrestricted_day_policy | 2,151/2,203 (97.6%) | 662/686 (96.5%) | 0.58 / 3.68 | 11.42 |
| valid | Balanced-4 cap 10 | 1,417/1,526 (92.9%) | 95/111 (85.6%) | 0.25 / 3.43 | 10.00 |
| valid | Balanced-4 cap 11 | 2,217/2,327 (95.3%) | 464/512 (90.6%) | 0.31 / 5.39 | 11.00 |
| valid | Balanced-4 cap 13 | 2,945/3,004 (98.0%) | 890/920 (96.7%) | 0.35 / 3.40 | 13.00 |
| valid | unrestricted_day_policy | 2,945/3,004 (98.0%) | 890/920 (96.7%) | 0.54 / 3.62 | 11.39 |

The production unrestricted profile preserves cap-13 coverage while saving 3,405 hires on dev and 4,736 on validation.

## All-expanded-set stress test

These rows deliberately ignore original hire count and are not the cap-coverage figures above.

| Cohort | Profile / cap | All days | Late days |
|---|---|---:|---:|
| dev | Balanced-4 cap 10 | 1,387/2,239 (61.9%) | 228/708 (32.2%) |
| dev | Balanced-4 cap 11 | 1,925/2,239 (86.0%) | 508/708 (71.8%) |
| dev | Balanced-4 cap 13 | 2,170/2,239 (96.9%) | 669/708 (94.5%) |
| dev | unrestricted_day_policy | 2,170/2,239 (96.9%) | 669/708 (94.5%) |
| valid | Balanced-4 cap 10 | 1,879/3,071 (61.2%) | 282/959 (29.4%) |
| valid | Balanced-4 cap 11 | 2,580/3,071 (84.0%) | 661/959 (68.9%) |
| valid | Balanced-4 cap 13 | 2,985/3,071 (97.2%) | 911/959 (95.0%) |
| valid | unrestricted_day_policy | 2,985/3,071 (97.2%) | 911/959 (95.0%) |

## Removed-restriction cases

| Cohort | Late seed/animal buy | Fertilizer buy | Fertilizer pickup | Hire after hour 1 | Any |
|---|---:|---:|---:|---:|---:|
| dev | 2,008/2,062 (97.4%) | 873/921 (94.8%) | 1,010/1,073 (94.1%) | 317/318 (99.7%) | 2,083/2,152 (96.8%) |
| valid | 2,744/2,820 (97.3%) | 1,154/1,218 (94.7%) | 1,355/1,431 (94.7%) | 420/427 (98.4%) | 2,867/2,953 (97.1%) |
