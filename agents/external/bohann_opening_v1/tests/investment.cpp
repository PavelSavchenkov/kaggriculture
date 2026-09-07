#include "support/adapter.hpp"
#include "agents/external/investment_context_guarded_001_best/source/agent.hpp"
std::unique_ptr<bohann_catalog_tests::AgentInterface> make_investment() {
    return std::make_unique<bohann_catalog_tests::Adapter<compositions::investment_context_guarded_001_best::Agent>>();
}
