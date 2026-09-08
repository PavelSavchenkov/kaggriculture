# Complete daily service on a larger cold farm

The dated search's p362 farm has16 animals and24 strawberries, but loses output
even after accounting for delayed births. Extract its accepted daily field work,
add missing useful CARE on animals already fed, and ask the persistent day solver
to complete all that work with the same workforce or one/two fewer hires.

Days14,18,22,26 use seed1000 seat0 against public_router. This is offline diagnosis;
full states and recorded rival actions never enter an executable policy. Check
both full-game action hashes and cash before the day comparison. A solved day
must match all requested next-day tiles, stocks, output, unchanged rival cash
and exact hire savings in the root engine, with no unit-action faults.

This extends the local cold composition work; no top-player route is imported.
Full-season integration and new-seed tests remain required before any promotion.
Run run.py via conda run -n kaggriculture. UNKNOWN is not infeasibility.

Result: all12 cases solved in0.066–0.113s and exactly replayed in the full engine. Same field work with two fewer hires saves233 on each of the four days. No extra CARE was eligible on already-fed animals in these samples. Profiles show missing FEED instead; the follow-up dated_day_service_sep08_002 adds that work using preserved wheat. Do not describe this first run as an output improvement. The initial compile used incorrect replay-result field names; that failed log remains saved, and the corrected run passed.
