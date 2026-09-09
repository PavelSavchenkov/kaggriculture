# Optional full research checkpoint

`ARCHIVE.json` records the exact checkpoint hash and inventory. The 489 MiB `checkpoint.tar.zst` is excluded from this source folder. It contains about 17.4 GB of raw evidence, schedules, attempts, training artifacts and history, useful for full reproduction and continued historical experiments. None of it is loaded during estimator inference.

The local retained artifact is `experiments/v6/fast_day_solver_estimator/distribution_archive/checkpoint.tar.zst`. It has not been published to an external artifact host. Supply it explicitly when restoring on another machine; the tool does not silently depend on the local experiment folder.

```sh
conda run -n kaggriculture python tools/restore_checkpoint.py /new/workspace --archive /path/to/checkpoint.tar.zst
```

Run from the estimator folder. The tool verifies both the archive and shared repository dependencies. Source-only use, development documentation, metrics, current model weights, example contracts and a full verified course remain available without this archive.
