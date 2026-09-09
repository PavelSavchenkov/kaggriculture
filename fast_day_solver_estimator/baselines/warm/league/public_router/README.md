# Public four-route agent

Typed C++ port of Thomas Tschinkel's public state router, downloaded September 7,
2026. Four replay routes; shop, carrot-price and milk-inventory branches;
no-op weed repair, same-turn shed projection, sell clamping and surplus sales.
Only active workers receive actions. Otherwise intended to preserve the donor.
No search or inferred opponent-private state. Fixed default 10x10, 720-step game.

See IMPORT.json for the exact notebook, source hash and route hashes. Original
episode/player identities for embedded routes are not supplied by the notebook.
Notebook-reported performance is not our evaluation. Current Kaggle rating is
unknown. Public source retained for provenance; user authorized component reuse.
Reference parity passed 8,628 actions, covering all four routes. It won
1,989/2,048 fresh promotion games against teammate_shoprouter and 98.4–100% in
256 discovery games against each of eighteen older C++ ports. Native shop RNG:
249/256 wins against teammate_shoprouter. Full composition search is unfinished.
The downloaded metadata does not supply an explicit license field; no license
for this notebook is inferred from another donor's notice.
