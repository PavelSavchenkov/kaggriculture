# Weights

`v12_cond/` is the network of every agent in this handoff (training: TRAINING.md section 4).

| File | Content |
|---|---|
| `model.bin` | native FP32 export read by the C++ agent (9,106,336 bytes, sha256 `4ef6b5fb1bc874de7ae9a32c42338026614c7933104eec13572fe57a8b853726`) |
| `model.pt` | PyTorch checkpoint for fine-tuning (`scripts/train.py --init weights/v12_cond/model.pt`) |
| `model.bin.condition`, `model.bin.features` | sidecars written by training (strength +150, day 41; feature version 4) |
| `history.json` | exact training arguments and the validation curve |
| `train_log.txt` | training log (final validation 17.229 at step 40,000) |

Other networks (v13-v18, seed replicates, zoo clones) were not better in games and are not included;
the six top-team clones used as sparring partners are in the team's Kaggle dataset
`pavelsavchenkov/kaggriculture-agent-zoo-sep25` (`training/models/zoo_*`).
