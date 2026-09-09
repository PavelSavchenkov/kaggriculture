# Rival wool purchase repair v1

Experimental, not promoted. Preserve rival_wool_context_v3. If its additional
wool continuation buys only one of two requested sheep at step217, retry the
missing purchase when funds and shed space permit, through step252. Its recorded
pickup is at253. Never retry across a day-end transition or after that deadline.
No new worker schedule, placement or service rule is introduced.

See IMPORT.json for the source course and diagnostic evidence. Actual funding,
storage, sales and future guard matches require full-game validation.
