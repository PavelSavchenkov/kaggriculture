"""Record the requested upload, unchanged research gates, and V25 source parity."""
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP = HERE.parent
old = (HERE / 'review_109.md').read_text()
table = old[old.index('| Metric |'):]
assert sum(line.startswith('| ') for line in table.splitlines()) == 84
stamp = datetime.now(timezone.utc).isoformat()
body = f'''# Review110 — {stamp}

The requested single upload has been made: Kaggle56101451, September8 15:30:57UTC,
animal_repair_q24_premium_m2. Archive SHA256
d65b8150db33becdedb4f752e8d773fe1c1b24306cb4459974104564d6bde374.
Kaggle initially reports PENDING; terminal status and independent validation
replay audit remain required. Monitor1064 is live. No further upload is authorized.

The user reiterated the outstanding submission request. The optional132864-game
study had delayed that request despite completed operational evidence. Select
the highest completed-discovery neutral win-score candidate for this upload:
93.262% versus81.445% for accepted parent, gain11.816pp with95% interval
[10.938,12.695]; mean-margin gain186.37 has interval[-89.54,604.82]. It ties the
premium/q24 variant on win score but has the higher mean margin. Preserve the
King tradeoff: -3.125pp and -9920.02 margin in discovery. This is a submission
decision documented in SELECTION.json, not a declaration that pending research
promotion gates passed. Those numeric gates remain unchanged. Accepted reference
is still empty_sale_slots_m2, cold reference early_melon_b98_m1.

Exact frozen C++ beats teammate4040/4096 and last submission3883/4096. Official
Python132 full games match94908 actions,128 final cash pairs equal native C++, and
27 rare-branch trajectories match19413 more actions. The frozen build reproduces
the submitted bytes. The server replay audit will test the artifact Kaggle used.

The experimental workforce caller fix is complete. It accepts and revalidates a
prior day schedule before escalating hires after UNKNOWN. Zero-new-search runs
retain all30 saved days across both courses and reproduce independent full719-step
results exactly: +178cash per case, hire cost5044 to4866, equal output and rival
cash. An all-PASS saved schedule with matching orders is rejected. Root day_solver
behaved according to its fixed-workforce API; no root edit or commit was made.

Public AhmedV25 is now a faithful C++ package in runs/ahmed_v25_sep08_001. The
original Chassis and view methods equal V23's AST; all four tapes equal Yusuke's
existing router. Source parity passes10793 actions with all four routes, threshold
edges,255 weed-repair changes,112 sale-leading changes and zero Python fallbacks.
Generic/pair/debug builds and self/PASS checks are running on4178. Competitive
evaluation remains due. Nusrati2715.6 is an exact four-file duplicate of the
already-ported Yusuke agent and needs no additional agent entry.

The original composition-first objective remains intact: lifetime search, fast
biology/economics/labor/funding estimates, exact schedules, deliberate service
exceptions, observed-shop and public-opponent branches, replay-derived ideas with
lineage, mixed/larger/cold farms and repeated league challenges. After completing
the server audit, finish the independent broad comparison and guard/tail review,
test V25 execution on the known compositions, then fold verified labor savings
into guarded continuations and repair mixedday10 funding/service. Do not shrink
the search to opening quantity or labor tweaks alone.

The82 metrics below reuse the latest15:08 global72-player/55-replay cohort and
localnative256 cohort. They are unmatched behavioral comparisons, not causal
effects. Final900000 remains unused; the unrelated GPU training is intact.
Next review15:50UTC, next replay/notebook refresh16:08UTC.

'''
(HERE / 'review_110.md').write_text(body + table)
note = f'\n{stamp}: Review110. Single authorized upload56101451 made15:30:57, PENDING; monitor1064 will fetch/reproduce server validation. Selected using completed discovery and exact package/native checks; broad132864 and unchanged promotion gates remain pending on62984, accepted reference stillempty_sale_slots_m2. V25 C++10793 source actions match, ops4178 live. Caller incumbent fix30/30days exact with zero search, invalid cache rejected. Nextreview15:50/refresh16:08.\n'
for name in ('PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md', 'NEXT.md'):
    with (EXP / name).open('a') as stream:
        stream.write(note)
print('Review110 recorded with82 metrics.')
