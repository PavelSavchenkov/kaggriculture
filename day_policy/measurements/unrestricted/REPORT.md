# Unrestricted day-policy measurements

Two sequential CPU14-pinned native Release runs; failed calls included. Late means days 20-28.

| Cohort | Profile / cap | All days | Late days | Median / mean ms | Mean hires |
|---|---|---:|---:|---:|---:|
| dev | Balanced-4 cap 10 | 1,387/2,239 (61.9%) | 228/708 (32.2%) | 3.34 / 14.70 | 10.00 |
| dev | Balanced-4 cap 11 | 1,925/2,239 (86.0%) | 508/708 (71.8%) | 0.41 / 10.68 | 11.00 |
| dev | Balanced-4 cap 13 | 2,170/2,239 (96.9%) | 669/708 (94.5%) | 0.35 / 4.24 | 13.00 |
| dev | unrestricted_day_policy | 2,170/2,239 (96.9%) | 669/708 (94.5%) | 0.61 / 4.47 | 11.43 |
| valid | Balanced-4 cap 10 | 1,879/3,071 (61.2%) | 282/959 (29.4%) | 3.71 / 14.89 | 10.00 |
| valid | Balanced-4 cap 11 | 2,580/3,071 (84.0%) | 661/959 (68.9%) | 0.47 / 11.26 | 11.00 |
| valid | Balanced-4 cap 13 | 2,985/3,071 (97.2%) | 911/959 (95.0%) | 0.35 / 4.29 | 13.00 |
| valid | unrestricted_day_policy | 2,985/3,071 (97.2%) | 911/959 (95.0%) | 0.61 / 4.51 | 11.41 |

The production unrestricted profile preserves cap-13 coverage while saving 3,405 hires on dev and 4,736 on validation.

## Removed-restriction cases

| Cohort | Late seed/animal buy | Fertilizer buy | Fertilizer pickup | Hire after hour 1 | Any |
|---|---:|---:|---:|---:|---:|
| dev | 2,008/2,062 (97.4%) | 873/921 (94.8%) | 1,010/1,073 (94.1%) | 317/318 (99.7%) | 2,083/2,152 (96.8%) |
| valid | 2,744/2,820 (97.3%) | 1,154/1,218 (94.7%) | 1,355/1,431 (94.7%) | 420/427 (98.4%) | 2,867/2,953 (97.1%) |
