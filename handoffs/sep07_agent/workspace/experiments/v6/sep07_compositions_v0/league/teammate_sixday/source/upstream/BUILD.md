# Rebuilding `agent.so`

The binary is committed (the repo force-adds agent `.so` files despite `*.so` in
`.gitignore`, matching `agents/shoprouter`). To rebuild it from the published upstream
source in `source/`:

```sh
cd source
g++ -O3 -std=c++17 -shared -fPIC -I. -o ../agent.so policy.cpp submission_bridge.cpp
```

`source/` is yhay81's published "Six-Day Public-State Agent Source" dataset, verified
against the SHA-256 hashes printed in the upstream notebook before use:

    agent_main.py  5809846fd528f00d1c01d6f58940633604adb7283d1e2f152603697967e3bb89
    policy.cpp     a3f04707615b7c04f3ab4a3c593081c98df2dd091be386aa80a3ffad7c00c1b4
    submission_bridge.cpp 2e58be38b2f67e9ed2cfaa97d0534285b2af21c7d4e2be92735e647d4a68a4c5
    policy_plugin_abi.hpp 67679edc55b1951deac859d1191a6eb2198868b3b7237beb2266db9788d80345
    pyrandom.hpp   3835fe39fbc74b05078022a839dea1c1aff7a9d6d21a1227d4d6cd61a3612d75
    sim.hpp        f483219c22dd9a05c00e28a200aa48d70d4a461915c09b3a0fb36c0330cc554c
    six_day_budget_guard.hpp 5cf24ed4099e5b19e9f195869881548f2145a40329fc30d11e682501c3688556

The locally built `.so` requires only GLIBC_2.14 / GLIBCXX_3.4.21, so it loads in the
Kaggle image despite being built with a much newer toolchain.
