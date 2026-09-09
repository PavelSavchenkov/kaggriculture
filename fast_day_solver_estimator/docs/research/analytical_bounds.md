# Necessary physical conditions

These checks apply to the public ordinary24-phase day contract, one empty farmer at dawn, the fixed non-hire purchases and the independently generated optional hire menu. They relax inventory use, tile timing and exact spawning. They do not certify feasibility.

Total work: every mandatory field action takes one worker phase. For each worker, shortcut its traveled path into a chain joining the shed super-root and its visited work tiles. The union of those chains connects every work tile. A minimum spanning tree on those terminals, with all four shed squares joined freely, costs no more than the total traveled path length. Thus mandatory actions plus that MST is a necessary aggregate worker-phase count. This argument uses total paths, not just the geometric union of their edges.

Distance shells: for work at shed distance at least r, each participating worker loses at least r phases before reaching any such task. Compare the number of those mandatory actions with the total remaining phases after releases and that travel relaxation.

Timed goods: for item i and hour h, subtract initial shed stock and purchases strictly before h from cumulative withdrawal demand. Ignore all other consumption. A positive remainder must come from field production. An output made at shed distance d cannot be deposited before hour2d+1, even with free choice among the four starting shed squares: d moves, its production action, d return moves and a deposit. Purchases at hour h occur after that hour's withdrawal checkpoint and cannot cover it. Internal reuse cannot create an alternative external source of the item.

If all eligible production quantities together are smaller than the remainder, the deadline is unreachable for any workforce under this contract. The implementation records the missing quantity separately from model predictions.

For reachable positive demand, divide each item's remainder by its largest single-action output and round up to lower-bound the required production actions. Find the smallest distance radius that could supply each remainder, allowing all outputs within it regardless of ordering. At least one returned path must cover twice the largest such radius, plus a deposit. Sum the action lower bounds and this one return requirement, then compare with worker phases available through h. This deliberately ignores many additional required trips/actions.

Workers carry unlimited goods, and remaining cargo transfers automatically after hour23. No artificial12-item carrying bound or obligatory day-end return is used. The route-packing and quantity/travel model features are heuristics and are not added to the proven lower bound.

The optional hiring rule may change spawn positions through shed occupancy. Bound calculations allow every worker to start at any shed square, which is a relaxation. Normalized source schedules are certificates only after replay under the new hire times.
