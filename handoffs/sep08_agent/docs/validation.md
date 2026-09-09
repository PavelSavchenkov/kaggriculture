# Handoff verification

- Verified57,808 snapshot records,34,689 distinct contents and11,664 committed
  repository dependencies before final metadata additions.
- All45 catalog agents:720 initial operational games, including self-play,
  PASS, typed-PASS/generic equality and one/multiple-thread equality.
- Reproduced384 full session match records across the current comparison set.
- Repeated8,192 exact frozen native matches: submitted agent wins4040/4096
  against the teammate and3883/4096 against the previous submission. Both
  complete action hashes, cash, output and selected diagnostics match.
- Added392 checks: debug self/PASS and exact generic records for every catalog
  agent,16 concrete candidate/teammate pair games, and16 cold-agent source matches.
- In a clean relocated repository-shaped tree with no live experiment folder,
  rebuilt the exact deployment archive and reproduced16 typed teammate matches.
- Rebuilt the animal compiler using committed root engine/solver dependencies:
  all12 construction fixtures,30 mixed-course endpoints and719 transitions pass.
  Mixed forecast V2 CSV is byte-identical; all9 physical production deltas match.
- The new paired comparison entry point completed a64-game two-opponent smoke
  test. Its negative q24 margin is retained. This is tool validation, not a new
  promotion or broad strength estimate.

See CATALOG_VALIDATION.json, DEBUG_AND_PAIR_VALIDATION.json, PORTABILITY.json,
COMPILER_VALIDATION.json, DEPLOYMENT_REBUILD.json and LOOP_SMOKE.json in evidence/.

The initial catalog build exposed aggregate value-initialization issues. A
forwarding default constructor fixes new package construction. Existing Titan
now explicitly invokes the same AgentCore(true) configuration. No strategic
setting changed. Debug and source-action parity checks include these agents.

The first repeated mixed audit attempted the original fixed output directory,
which correctly refused to overwrite archived results. tools/check_compiler.py
uses fresh outputs and reproduces the full result. The source solver was not
changed.

All source/evidence blobs are lossless. New large profiles were recompressed
from gzip to xz after a measured45.9MB profile shrank from9.26MB to0.33MB in an
initial comparison; the retained compressor uses its documented faster preset.
No engine or day-solver source copy was added to this handoff. Full original
JSON can still be restored when needed.

The full132,864-game broad analysis was also rerun from restored evidence and
reproduces the complete original result object exactly, including failed gates.
See evidence/BROAD_REPRODUCTION.json.
