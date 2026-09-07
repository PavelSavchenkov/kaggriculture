# Agent Instructions

These instructions apply unless explicitly overridden.

## Code Style

- Keep code concise and names readable; follow KISS and Occam's razor.
- In Python, use direct imports; no `try`/`except` import fallbacks.
- Fail fast on unexpected states; add fallbacks only when strongly justified.
- Omit `-> None` from functions that return nothing.
- Do not add `__init__.py` files.

## General

- Run all Python, Kaggle, notebook, and package-management commands via `conda run -n kaggriculture <command>`.
- Do not use Git.
- Implement local agents only in C++. Read and follow `prompts/local_agent.md`.
- Unless already storing files in `experiments/`, store work scripts and artifacts in `work/`. Do not pollute the repo root and unrelated folders.
- When working on an experiment in `experiments/`, read and follow `prompts/experiments_pipeline.md`.
- Keep `experiments/<version>/<date>_<name>/` self-contained: store all experiment artifacts there and never reference another experiment. It may use `agents/` and persistent root resources, such as knowledge and the game engine; create a new agent in `agents/` only when explicitly instructed.
- Store every submitted agent artifacts in `submissions/[date]-[submission-name]`. e.g. `submissions/aug27_baseline_simple/`. It should be self contained (e.g. include copied agent source from `agents/`).
- Digestable game rules are in `prompts/game_rules.md`. Read it.

## Language in docs and chat

- When asked for markdown in chat interface, provide it as unrendered plain text.
- Use the simplest concrete wording that preserves the full meaning. Do not add abstraction, jargon, or explanation unless it improves precision or avoids repetition.
- Prefer direct, practical sentences. Remove wording that does not change a fact, decision, requirement, metric, or action.
- Example: replace “The blind novelty reserve remains unprofiled” with “Keep some opponents unused and unanalyzed until the final agent is ready.”
