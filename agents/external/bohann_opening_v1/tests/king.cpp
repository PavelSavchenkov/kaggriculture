#include "support/adapter.hpp"
#include "agents/external/king_rc4/source/agent.hpp"
std::unique_ptr<bohann_catalog_tests::AgentInterface> make_king() {
    return std::make_unique<bohann_catalog_tests::Adapter<compositions::king_rc4::Agent>>();
}
