# Public source changes, 06:08 refresh

- fierus/xray-kaggle: changed from42 prior function definitions to20 different
  replay-analysis functions. It is an analysis notebook, not a deployable agent.
  Its discussion of yamakawanin submission56087555 distinguishes live replay
  behavior from the public q30-v2a file. Its published metrics are unverified
  here; use our engine/replay accounting for actual executed quantities.
- tokenjunkielabs/tokenjunkielabs-farm-manager: now retrieves the pinned TITAN
  archive at commit7c50bbfb41027f31a2d4bc9470424e815f1fcef1, SHA256
  7b58fa06da778b1519b81d509d28dff3481b3bbcc7a2d656e8bdfe4a22540524.
  Archive286356bytes/78files verified and safely extracted; IMPORT.json lists hashes.
  Default frozen SELL path uses Arlene production with seed/funding enabled,
  terminal_route/history disabled. Source reports ancestor-only game evidence.
  Do not infer current packaged or league strength from it.
- Useful component: scheduler.py MarketPath and optimize_lot search integer
  partial sales over at most8turns and compare own-minus-rival receipts under
  no-rival, simultaneous, later-order, next-turn and late-batch scenarios. Quotes
  and $1 inventory admission are exact within those conditional streams.
  Cash/storage/arrival feasibility must be supplied by the caller. Preserve all
  order positions when delaying sales. Source attribution/licenses remain in
  the pinned archive; C++ core work is runs/titan_sale_lots_sep08_001.
- Downloaded yamakawanin/kaggriculture-2312-9-q30-v2a from the Xray reference.
  Source extraction and comparison with our older King RC4 port remain pending.

All notebook code was treated as source data. Only selected reviewed pure
mathematical definitions are used by the isolated lot-search parity oracle;
no notebook, loader, full external agent, submission or author workflow ran.

Follow-up: King q30 source assembled from12 writefile cells without executing them. Its52 PROGRAM blocks and719 MARKET_LIB records equal the previousRC4source. All shared functions except the effective structural repair match; oldRC4 has an additional later PLANT-on-WEED repair, q30 only the older BUILD repair. Later RC4 overlays are absent. See king_q30/SOURCE_AUDIT.json and STRUCTURAL_RECOVERY.diff. The assembled source hash differs from Xray’s cited source, so no byte-identity claim is made. TITAN C++ lot core passes216 cases and benchmarks50.35us/context; integration remains next.
