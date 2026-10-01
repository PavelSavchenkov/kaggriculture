# Transformer opponent forecast (vendored)

Copied Sep 26 21:22 from work/sep26_wide_losses (agent-losses session): probe/fc_transformer.hpp, probe/fc_features.hpp,
weights fcexport/tf_b_cos.bin (copied into model folders as model.bin.forecast_tf). Hooked by scripts/dc11_local_additions.py step 4.

SHA256:
186d94b58fa0a0bbe8a14bbd0895d3b161bce91195f6628582ffabb001d786cc  dc11_local/fc_transformer/fc_features.hpp
7c844765861977312a52e5007649bafa450b5eb4f6952445502fcab24ed93ae6  dc11_local/fc_transformer/fc_transformer.hpp
bd353f110fa36bd45d0e7db50e54b12c21055d1704323d24b9f37c58e905f22a  /home/pavel/Programming/kaggriculture/work/sep26_wide_losses/fcexport/tf_b_cos.bin
tf_lin_w1_l5.bin sha256 f6763d1adcd6c903695415f5a656e21e548270932afe3e8ec316727c9cef329c

Updated Sep 26 23:45 from work/sep26_wide_losses/probe/fc_transformer.hpp (intra-day head: intraday(), has_intraday();
FCT2 files with the xlag input; old files load unchanged). Models: intra_w1_l5.bin, onp_x_w1_l5.bin (fcexport/).
dffb2bd2945ec699c973c28c351f270cf14ccd8f7e098daf5403536b15a0b945  dc11_local/fc_transformer/fc_transformer.hpp
cccd788964d8c0074e9ca6a1c089fd3cea149ff0bd192085defcb21fdeaaf4b6  fcexport/intra_w1_l5.bin
2274403990ddb5d0c7aec7dc1b4037a44b826f973157d884b371dd31516e1898  fcexport/onp_x_w1_l5.bin

Updated Sep 27 02:59 from work/sep26_wide_losses/probe/fc_transformer.hpp (FCT4: stock / visible / xseen inputs of the intra-day head;
older files load unchanged). Model stk_xs_w1_l5.bin (fcexport/).
a7470fa876f98afd896fc466959024eb8f2305eeeaaac0d31e17ca55318cf057  dc11_local/fc_transformer/fc_transformer.hpp
c4ecad75ae5c15a35b264806e432416a186f2d4061c308c95f154b74198eb72e  fcexport/stk_xs_w1_l5.bin

Updated Sep 27 19:46 from work/sep26_wide_losses/probe/fc_transformer.hpp (FCT5: the intra-day head also predicts the opponent's first
next_hours() hours of tomorrow, intraday(..., next_out); FCT4 and older files load unchanged, next_hours() = 0).
c0faa841e6f3fba5fc13a0bc755ea4e941532277ca29bad0813265ac7cd8b915  dc11_local/fc_transformer/fc_transformer.hpp
