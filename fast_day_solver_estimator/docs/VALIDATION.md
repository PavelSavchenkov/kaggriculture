# Delivery validation

These checks validate the handoff. They do not promote a model or add a new performance result. The original delivery contained vendored dependencies and the complete archive. The lean source package now uses identical shared repository dependencies; its fresh checks are recorded in `evidence/LEAN_PACKAGE.json`.

- The public C++ API compiles without the scheduling backend. CTest checks worker-count input isolation, zero marginal change and analytical rejection.
- The public CLI handles an ordinary H24 contract, terminal H23 contract, and a zero-hire menu. Rejected point estimates are JSON null, not invented costs.
- Frozen cold and warm gate reports pass consistency and arithmetic checks.
- The relocated published agents produce the original fixed-seed source season exactly: both action hashes and final cash match, and all 90 day contract/action files are byte-identical.
- A saved successful guided course is independently replayed for all 719 real transitions, matching every field of its original verification report.
- In the original portable delivery, runtime checks were repeated after copying the package and pointing warm targets at its vendor snapshot. The lean package is checked from a fresh minimal repository containing only the estimator and the explicitly listed shared dependencies. Compiler and OS requirements remain external.
- Generic and typed pair fixture runners perform complete source-versus-rival, self-play and PASS games, in both seats on two seeds. They check action metadata, repeat each game after resetting the same instances, and run separate instances concurrently. Their action hashes and rewards match exactly. Validation assertions remain enabled in the runners.
- The complete checkpoint archive passes SHA-256, zstd stream and member-path checks. It has been restored into a new workspace, where the original research CMake project builds reference and planning-prediction tools.

Primary original records are `evidence/package_check_v1/CHECK.json`, `evidence/isolated_check/CHECK.json`, `evidence/FIXTURE_AGENT_PARITY.json`, `evidence/FIXTURE_INTERFACE.json`, `evidence/ISOLATION.json`, `evidence/RESTORE_VALIDATION.json`, and `research/ARCHIVE.json`. Historical paths in these records describe the original portable delivery. Current dependencies are in `REPOSITORY_DEPENDENCIES.json`. Representative input/output fixtures and the fixture runners are included; build logs, duplicate source-season exports and bulk model-prediction dumps are retained with the experiment backup.

Run `conda run -n kaggriculture python tools/check_integrity.py` to verify the delivered files against `MANIFEST.json`, and `conda run -n kaggriculture python tools/repository_dependencies.py` for shared dependencies. Build directories, Python caches and the optional research archive are excluded from the source manifest. Extra files from later runs are allowed, but listed delivery files must still match. Use `--write` only when deliberately making a new delivery; it replaces the recorded hashes.

New isolated binaries are functionality checks, not byte-identical rebuilds of the historical binaries. Frozen snapshots preserve the historical code, compiler flags and hashes. Different flags, hardware or bounded parallel backend behavior can change timing and search outcomes.
