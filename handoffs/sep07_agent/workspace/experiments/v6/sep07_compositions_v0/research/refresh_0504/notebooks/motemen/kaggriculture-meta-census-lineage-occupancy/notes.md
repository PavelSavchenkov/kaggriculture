# Kaggriculture Meta Census — Lineage Occupancy

Who is farming what on the top of the ladder, and how fast does the meta rotate?

This notebook tracks **agent lineages** in the official
[Kaggriculture episodes datasets](https://www.kaggle.com/datasets/kaggle/kaggriculture-episodes-index)
(a daily excerpt of top-band games) and charts their **occupancy over time**.

## Method

- A seat's **lineage** is the SHA-256 (first 10 hex chars) of its first 24 steps
  of actions — the "opening signature". Deterministic agents produce a stable
  signature across towns and seeds, so identical-opening agents cluster
  together even when their mid-game behavior adapts.
- **Occupancy** = share of seats per day carrying each signature.
- A lineage is **labeled with a public notebook's name only when that
  notebook's extracted agent reproduces the signature in a local mirror match**
  (a verifiable claim anyone can replicate). Everything else stays a bare hash
  — no guessing about private agents' origins.
- Earlier days are carried as pre-computed counts (same pipeline, run daily);
  the attached day is mined live from the raw episode JSONs below.

The result is a picture of the meta as an ecosystem: public-notebook waves
sweeping the ladder in days, counter-strategies rising, and the occasional
unlabeled newcomer climbing out of nowhere.
