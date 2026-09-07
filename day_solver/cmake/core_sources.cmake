# Project sources corresponding to the bundled V30 static archives.
set(DAY_SOLVER_CORE_SOURCES
    native_quick_portfolio_v30/quick.cpp
    native_ranked_neighborhood/neighborhood.cpp
    native_ranked_neighborhood_public/api.cpp
    native_ready_screen/screen.cpp
    native_quick_portfolio_v11/quick.cpp
    native_public_portfolio/portfolio.cpp
    native_constructor_data/vendor/rectangular_lsap.cpp
    native_semantic_control_v2/controller.cpp
    native_semantic_context/context.cpp
    native_early_completion/completion.cpp
    native_solver_api/exact.cpp
    native_solver_api_coarse/screen.cpp
    native_exact_domains/solver/src/problem_json.cpp
    native_exact_domains/solver/src/problem_validation.cpp
    native_exact_domains/solver/src/invariant_requirements.cpp
    native_exact_domains/solver/src/tile_graph.cpp
    native_exact_domains/solver/src/baseline_json.cpp
    native_stock_screen/screen.cpp
    native_unnamed_exact/exact.cpp
    native_light_exact/exact.cpp
    native_geometry_screen/screen.cpp
    native_materialized_screen/screen.cpp
    native_light_screen/screen.cpp
)
list(TRANSFORM DAY_SOLVER_CORE_SOURCES PREPEND "${DAY_SOLVER_COMPONENTS}/")
