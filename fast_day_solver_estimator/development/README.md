# Research source snapshot

`scripts/` is provided for browsing the offline training, extraction, evaluation and export pipeline. It retains the experiment's path assumptions. Supply the separate checkpoint archive to `tools/restore_checkpoint.py --archive /path/to/checkpoint.tar.zst <new_workspace>` and execute scripts from the restored experiment; that layout includes all required datasets, raw attempts, schedules and snapshots. The archive is optional for inference and included runtime checks; see `docs/DEVELOPMENT.md` for how to obtain it.

The root package's supported entry points are the C++ API, `estimate_day`, the CMake reference/replay targets, and `tools/`. See `docs/DEVELOPMENT.md` for precise objectives, continuation rules and promotion artifacts.
