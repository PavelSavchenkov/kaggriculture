# Fresh Yusuke confirmation

28672complete games;512fresh seeds/both seats; four agents against seven
matching opponents. All271frozen input dependencies remain unchanged.

| Opponent | Yusuke full W/T/L | Current W/T/L | Yusuke margin | Current margin |
| --- | ---: | ---: | ---: | ---: |
| empty_sale_slots_m2 | 1024/0/0 | 81/862/81 | +7598.79 | +0.00 |
| teammate_shoprouter | 1006/0/18 | 1008/0/16 | +10722.14 | +11687.11 |
| public_router | 959/0/65 | 976/0/48 | +6821.37 | +6870.67 |
| public_router_v52 | 758/1/265 | 925/0/99 | +2956.21 | +4013.37 |
| ahmed_v23 | 752/0/272 | 892/0/132 | +2831.24 | +3518.94 |
| junghoon_wool_sales | 976/0/48 | 967/0/57 | +16856.84 | +16851.49 |
| king_rc4 | 918/0/106 | 1019/0/5 | +10218.88 | +21456.48 |

Yusuke wins1024/1024direct games against the current reference, mean+7598.79.
However, against the six neutral opponents, current utility is
94.189% versus 87.394% for Yusuke.
The paired Yusuke-minus-current gain is -6.795pp,
95%seed-cluster interval -8.268to-5.412pp.
Mean margin gain is -2331.90.

The seven-opponent average includes current self-play and therefore gives the
new counter a large structural advantage on that row. Report both groups;
do not use that average alone to promote a replacement. Keep current reference,
add this strong public opponent, and improve the exposed matchup with broad
regression checks. Yusuke's early-only and local guarded variants also win
every direct game but do not solve its broader deficits. The local guard is
not selected over the faithful source.

Same-game gap_diagnostic identifies production and execution differences.
The offline opening witness in ../animal_group_policy_sep08_001 isolates a
first-turn price/funding effect. Copying both market turns is harmful; changing
only the first round-trip quantity helps in one exposed world. New complete
strongest-parent variants are in ../opening_funding_sep08_001 and still require
operational and broad league checks. No promotion or external submission.
