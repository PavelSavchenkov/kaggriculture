#include "support/adapter.hpp"
#include "agents/external/public_router_v5/source/agent.hpp"
std::unique_ptr<bohann_catalog_tests::AgentInterface> make_router() {
    return std::make_unique<bohann_catalog_tests::Adapter<compositions::public_router_v5::Agent>>();
}
