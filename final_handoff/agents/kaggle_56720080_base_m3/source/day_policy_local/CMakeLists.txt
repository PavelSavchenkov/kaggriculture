# Experiment-local copy of the repository's day_policy route solver (source/ copied on
# 2026-09-25 from REPO/day_policy/source; hashes of the copied files in
# SOURCE_COPY.sha256). Local changes: SolveOptions::min_hires (warm start for re-solves).
add_library(contract_policy STATIC source/agent.cpp source/routes.cpp source/policy.cpp source/verify.cpp)
target_include_directories(contract_policy PUBLIC "${REPO_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}/source")
target_compile_options(contract_policy PRIVATE -Wall -Wextra -fno-exceptions -fno-rtti)
add_library(day_policy::policy ALIAS contract_policy)
