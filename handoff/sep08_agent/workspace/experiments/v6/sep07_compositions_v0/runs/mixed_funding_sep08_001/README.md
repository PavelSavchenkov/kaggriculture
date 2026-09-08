# Mixed-farm funding continuation

A day9 mixed change puts a goose on tile5 and sheep on tiles7/8. Its altered
cash flow leaves the inherited day10 order for two further geese only partly
funded. The original route requests the missing goose from the shed and later
tries to PLACE/feed/care at tile14; all three fail. This is not missing pasture
construction or insufficient physical workforce.

The offline diagnostic keeps all worker actions and splits the two-goose
purchase:one at hour1,one at hour10 (hour12 also works). It replays both the root
physical contract and the actual reacting-opponent day. Endpoint now matches;
end wheat59->58, second goose placed/cared, cash12866->12566 pays that goose.
CSV markets.actual_quantity covers product trades; fixed_paid records animal,
seed and hire payments. In particular300 paid for a two-goose request means
only one goose bought. Hour cash columns bracket the whole turn, not each slot.

Source/lineage and exact results are in LINEAGE.json and day10/. No downloaded
notebook code was executed or copied. The nearby public carried-animal repair
idea is recorded separately with attribution, but would not fix this cause.

resume.py combines certified days9/10 and invokes the existing flexible
compiler on days11-29. All 21 changed days compiled. audit.py independently
replayed all 719 transitions and matched all 30 day endpoints, final cash and
both players' production. RESULTS.json and audit/ hold the exact evidence.

This fixed-world course produces 36 more eggs, 44 more wool and 57 more
fertilizer, while displacing 16 wheat, 7 carrots and 14 strawberries. Labor
cost rises by $1,563. Our cash falls $4,923 (100387 to 95464); the opponent's
falls $4,956 (90444 to 85488), so the margin improves only $33. This is one
offline composition certificate, not a deployable-policy or league gain.

The compiler must recheck funding of inherited purchases after changing an
earlier composition. A physically valid route can still fail when the animal
was never bought. Splitting a purchase before the actual pickup repaired this
case without changing the worker schedule.
