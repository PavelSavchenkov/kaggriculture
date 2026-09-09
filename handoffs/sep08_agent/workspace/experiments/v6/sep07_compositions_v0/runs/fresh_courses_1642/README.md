# Fresh complete courses, September 7 16:42

The latest top-12 cohort contains 72 player-games from 62 raw replays. This run imports 69 distinct new complete courses into a separate C++ library, retaining every donor, episode, seat, submission, snapshot and source hash. All 49,611 translated actions match their original observations. Existing libraries and submitted agents are unchanged.

PLAN.md defines the search. library/IMPORT.json gives exact attribution. prepare.py formats immutable data and performs full source parity. build_screen.py builds the C++ screen, whose command and inputs are in build/build.json. Run the screen binary with a new output directory, then analyze_screen.py for the saved discovery matrix.

SCREEN.json covers 26,496 candidate games plus 384 current-agent controls: 32 seeds, both seats, six opponents. No raw new course beats crop_mix_t2_wheat directly. The best whole-league course, Bohann Wang program25, reaches 58.33% equal-opponent utility but still loses $1,769.76 mean paired margin relative to current. It wins 62/64 against King RC4 while losing most direct current-agent games. This is a useful specialist and composition donor, not a promoted policy.

Program25 is episode106497007 seat0, submission56071218. It produces much more carrot, tomato and egg than the current family. Its source opening differs in the first two market actions, while its physical farm matches the original Justin150 course at dawn1 and4–6. PREFIX_CHECK.json is a restrictive raw source-state comparison, not proof of compatibility with current runtime execution. Four complete component ablations separately test the new opening markets and whole continuations from dawn1 or6. Exact physical entry guards cover all100cells and private own stocks. See ABLATION_LINEAGE.json and proposals/. A selected full continuation receives the existing terminal recall/liquidation; this is an explicit locally borrowed component.

The only updated notebook in this refresh is Destbreso's X-ray. Its executable AST and all42functions exactly equal the saved14:46version. It is a replay diagnostic, not an agent. notebooks/destbreso_xray/AUDIT.json retains the comparison; notebook code was not executed.

Discovery does not establish performance on unused seeds. Reserve1750000for any later fixed candidate, and retain the final900000pool. All Python, builds and binaries run through conda run -n kaggriculture. No further Kaggle upload is authorized.
