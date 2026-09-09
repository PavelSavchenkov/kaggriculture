# Seed and land release conditions

This development calculation is separate from the frozen second-wave predictor and does not alter its prospective test.

The inputs are a validated day problem, its permitted hiring menu, and 23 or 24 active phases. The calculation never reads the source workforce or schedule. Birth time is hire hour plus one; the farmer is present at phase zero. Distance is the minimum Manhattan distance from any shed-access tile, deliberately relaxing actual spawn positions.

For seeds, count required plant actions at distance at least r, separately by crop. At each seed purchase release t, subtract initial seeds and all purchases released strictly before t. Sum positive deficits across crops. At least this many plant actions must occur at t or later. No pickup is needed because seeds are shared global inventory. Surplus early seeds for one crop cannot cover another crop's shortage.

For land, every farm action on a tile initially locked must wait for its quadrant purchase to become usable. At each phase t and radius r, count required actions on initially locked tiles whose land release is at least t and distance is at least r. Missing land purchases or purchases in the last active phase make any required farm action there unreachable. Owned tiles impose no land-release restriction.

Workers can move through locked land and wait before seeds or land become available. A worker born at b can optimistically reach radius r at phase b+r. Its capacity after release t is therefore max(0, H-max(t,b+r)). This permits every subsequent action to fulfill the selected cut, ignoring all extra movement and prerequisites. The sum across k workers must cover the cut. The result is the maximum necessary workforce over all cuts; seed and land counts are not added together because they can refer to the same action.

Required actions in a tile chain are not treated as separate phases: multiple workers may plant and water the same tile consecutively within one phase. The capacity calculation counts actions, not chain duration. Engine controls include that case on the last active phase, with workers prepositioned before the land purchase.

A workforce bound of 41 without a missing quantity means only that the selected 40-worker menu fails this relaxation. It is not a claim of impossibility with unlimited workers. A positive seed or land missing count is a proof of an unavailable necessary resource. The seed calculation also checks total stock after all purchases; terminal inventory requirements and other resource interactions remain relaxed.

First audit: 88 boundary controls pass. Across 6,205 input variants representing 3,739 physical contracts, 4,445 verified witnesses produce no contradictions. Ten other historical witnesses retain their previously known replay failures. Mean call time is 1.863 us and p95 is 2.824 us, including the hire menu but excluding parsing. On the five older development panels, the condition adds no pruning beyond the output, carried-input, and workforce-capacity bounds already present. This is a negative usefulness result; the condition is not added to the frozen predictor. New expansions and delayed calendars remain to be tested.
