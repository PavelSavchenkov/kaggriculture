# Public skomuro 2,000+ baseline C++ opponent

Opponent-only optimized C++ port of public Kaggle notebook
`skomuro/2000-baseline-silver-medal-route`, version 2 public run
`345934372`. The downloaded notebook SHA-256 is
`07be33e7d31cf553336329c572bcc8e67229083dd96375d4858ebedf7cb4560d`.

The port preserves the source's 720-row canonical tape and every enabled final
overlay: per-worker weed delay, premium-cargo banking, first-two-shop
cow-to-sheep conversion, staged surplus-wool sales, one-step wheat/fertilizer
sale debt, late liquidation, terminal cargo return, feed-order placement, and
premium impact ordering. The source's final cell disables mirror front-running;
the port therefore omits that inactive branch. Runtime uses fixed arrays,
bounded order buffers, numeric enums, and no heap allocation in `act()`.

The port matches the mechanically extracted Python source exactly in five
full PASS games (`1`, `740000018`, `750000001`, `750000002`, and `990000007`):
all 719 requested actions, all eight random shop draws, and final cash agree.
The accidentally touched `750000002` block was invalidated before any candidate
use. The fixed-array replacement also reproduces the pre-optimization strict
flow CSV byte for byte.

Strict, native/LTO, and ASan/UBSan flow profiles are byte-identical over all
4,096 four-shop sequences and both seats (CSV SHA-256
`5619535d3e9369039a49e747249d6dccc99603689bbc46d8a31413d642f8bcfc`).
The action path contains no `vector`, `string`, heap allocation, or standard
stable sort/partition; fixed arrays, insertion sort, and bounded manual
partitioning replace them.

This opponent is admitted only for outcome pressure. Its source-faithful
invalid requests disqualify it from correctness evidence.

The public notebook page did not expose a license label through downloaded CLI
metadata. Keep this derivative inside the experiment as an evaluation opponent
and do not use it in a submission. Its source, actions, routes, thresholds, and
constants must never inform candidate policy rules.
