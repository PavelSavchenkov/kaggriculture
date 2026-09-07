Animal decisions in 51 recent strong-player games

All times below are zero-based engine day/hour. This is replay evidence, not a counterfactual win-rate test. The sample has seven players, each observed under one submission version, with ranks 1–12 in the saved fresh leaderboard cohorts. The extraction records 624 orders, 847 successful animal purchases, 843 successful placements, and exact already-revealed shops. Source replay hashes and exact decision steps are retained in every event row.

1. The strongest reusable suffix is Atakan's three-way late-animal branch.

Submission 56058092 has identical raw and normalized actions through step225 in episodes106371597 seat1 (cow),106380079 seat1 (sheep), and106374592 seat1 (goose). At step226, day9/hour10, the same worker commands and sale/feed orders accompany a different one-animal purchase. Another two of the selected species are bought at241. They are placed at258/259/260 on (1,4),(3,2),(4,1), previously melon tiles. Construction at257/258/259 changes between pasture and coop. The separate sheep at (2,3), placed270, stays sheep in all three courses.

Cow versus sheep has exactly equal own public farm and ordered private state except cash. Cow versus goose additionally has shed WHEAT28 versus20; all other own physical/private values agree. Markets, opponent farms, shops, and cash differ and are preserved in exact_cases/. This is good evidence for reusing a common opening with alternative physical suffixes. It does not identify the hidden selection formula or establish that any suffix is stronger on a different market.

At226 the observed features are:

| Episode | Species | Already-revealed shops | Milk/wool/egg demand | Milk/wool/egg quote | Cash before animal order |
| --- | --- | --- | --- | --- | --- |
|106371597|COW|Ice Cream, Bakery, Smoothie|2/0/1|206/172/52|1281|
|106377266|COW|Bakery, Ice Cream, Ice Cream|2/0/1|187/172/53|1203|
|106380079|SHEEP|Pizza, Pet Cafe, Yarn|1/2/0|203/196/51|1498|
|106374592|GOOSE|Farmers Market, Pet Cafe, Pizza|1/0/0|137/189/51|1295|

Two simple legal-feature hypotheses fit these four observations: prioritize sheep when Yarn has been revealed, otherwise cow if observed milk demand is at least2, otherwise goose; or prioritize sheep above a wool-price threshold195, otherwise cow above milk-price180, otherwise goose. These thresholds merely describe the sample and should compete with economic valuation in local tests. No formula has been proven. The goose case has zero revealed egg demand, so buying only the species with greatest shop demand would not reconstruct every strong-player decision.

The three exported courses are existing library programs156/160/157. atakan_three_way/ contains all719 raw actions, exact existing-library typed encoding, normalized active-worker actions, exact before226 states, dated crop/animal lives, tile-level successful service events, realized harvest, trades, composition intervals, and complete dated day templates. MANIFEST.json has all source/artifact hashes. Harvest is not gross production; caps, failed collection, and exits can separate them. The exact day states are retained for further production accounting.

2. Early three-way selection and a smaller cow/sheep substitution are also visible.

3정훈 submission56063995, rank2, buys two cows at150 in two games, two sheep in two, or two geese in two. Only the first two shops have been revealed. Both sheep cases contain Yarn. The goose cases are Pet Cafe/Brunch (egg1,milk0) and Bakery/Pizza (egg1,milk1); the cow cases have milk demand1 or2 and no egg/wool demand. This fits a Yarn-first, otherwise egg-versus-milk hypothesis, but the sample is too small to establish it. Cow106385432 seat1 versus goose106398132 seat0 has exactly equal own physical/private state except money at150 despite earlier sale-action differences. Both use (5,3),(5,4),(6,4),(5,2), with pasture/coop changes. The sheep course106390091 seat1 already differs in workers, tiles and private inventory by150, so it requires an earlier compatibility boundary or scheduling repair.

Mengfei Li submission56047440, rank2, keeps the cow batch at150 in all six games. At169/176 it buys sheep in both games with revealed Yarn (106373411 and106377269, seat0), and cows in the other four. Placement can reuse pastures. The sheep/cow pair106373411/106381779 has equal own farm except cash at169, but different shed inventory; exact own actions first differ at168. This is an especially small alternative to changing the entire composition opening.

JustinLee submission56065461 buys sheep at150 in all six observed games whose first two shops include Yarn, and cows in the six without Yarn. A further recorded suffix106415893 seat1 buys cows at217/226 and sheep at241/265, while the current source106370340 seat0 buys sheep then geese. Their own physical/private states agree except money at196, although their sale quantities then diverge. These are observed branch candidates, not proof that every difference is caused by shops.

3. Purchases shortly after reveals can have substantial execution delays.

There are32 successful order records one turn after a shop reveal and eight at step360 or later. Timing alone does not prove intentional waiting.

ymg_aq, rank1, episode106402121 seat1 buys four sheep at361/362 after the first Yarn reveal at360. There are no ready pastures and no held sheep before361. Placements occur at406,439,447,447 after wheat/melon exits: the batch waits44–86 turns before production can begin. Its current wool quote is only55 at purchase despite new future demand. Episode106411219 seat0 buys a goose at505 after a Brunch reveal at504, then places it at511 after a melon exits508 and a coop is built510. Episode106415741 seat0 buys a goose at433 after a Brunch reveal432, but does not place it until496 after a strawberry exits488. These are actual late investments; their profitability and whether delay was deliberate are untested.

自己找差距 episode106392884 seat1 buys five cows at361 after the fifth reveal makes observed milk demand4. No pasture is ready. It places them375/377/378/381/382 on former melon→wheat tiles. This provides a concrete batch-purchase and crop-to-pasture suffix for a compiler. It also shows why purchase day is not an adequate proxy for productive start day.

4. Reuse and negative findings constrain the estimator.

192 of843 placements reuse former crop tiles.48 have a closer ready structure and88 a closer buildable site according to the existing sequential action audit. Other workers may already be committed to those sites, so these counts do not prove inefficient placement. A center-distance-only rule will not reconstruct all recorded choices; crop exit dates, structure compatibility and worker routes matter.

Two cases replace an empty coop with a pasture: 自己找差距106392679 seat0 at (4,0), DIG304/BUILD_PASTURE305/PLACE_SHEEP306; and106392884 seat1, DIG298/BUILD_PASTURE299/PLACE_COW300. The coop originally followed a melon crop and had never held a goose in the observed timeline. One case reuses an existing vacated pasture:3정훈106396570 seat1 places a new cow at310 at (5,3), whose earlier cow disappeared at the update producing state216. This observation does not establish that the earlier escape was intentional.

All11 positive animal orders that failed are cash-limited. Atakan attempts a cow at64 with219–226 cash in all six games, then succeeds at65; this is a failed request followed by retry, not evidence of deliberately waiting for a shop. Justin106419058 seat0 fails five animal orders with18–213 cash, while Bohann succeeds on the same shop sequence from seat1. There are also16 explicit zero-quantity sheep orders, including step217 in every Mengfei game. Some occur with enough cash for a sheep, but their repeated fixed position does not establish an information-seeking wait rule. No intentional-wait policy or counterfactual payoff has been proven by this analysis.

Recommended experiment order: reconstruct Atakan's three suffixes from the verified common boundary; compare demand and marginal economic selectors; carry acquisition, placement, crop-exit and service dates separately; then test late additions with full scheduling costs. Keep branch formulas and wait decisions as hypotheses until same-seed C++ comparisons support them.
