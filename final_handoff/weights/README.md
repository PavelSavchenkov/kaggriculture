# weights/

Every distinct binary model file of the final agents, stored once (121 MB). `SHA256SUMS` lists them. Each agent's
`WEIGHTS.txt` maps its model files to these, and `../agents/assemble_agent.py` copies them into place with hash checks.

| Path | What | Used by |
|---|---|---|
| `network/v17g6ft5/model.bin` | Main DayIntent network | all four agents (`model/model.bin`) |
| `network/members/{w384b,sw100,mw5k,mw2k}.bin` | Ensemble members (v12 era) | all four (`model/members/`) |
| `forecasters/big2_m3/forecast_tf.bin` | Opponent-sales forecaster big2_stk_xs | base m3 |
| `forecasters/fc2/seed{0,1,2}.bin` | fc2 (includes our own games in training) | f1 (3 seeds); seed 0 alone in Kaggle 56714867 / Local-LB #2 |
| `forecasters/fc3nvens/seed{0,1,2}.bin` | Clean forecaster (no own-lineage games, no bed worlds) | honest1, m3_fc3nv |
| `forecasters/next_morning_head/forecast_tf_next.bin` | Next-morning head | all four |

The text sidecars that complete each model (`.condition`, `.features`, `.decode`, `.dc11`, ...) are in the agent folders.
Provenance and evidence: [../05_MODELS.md](../05_MODELS.md).
