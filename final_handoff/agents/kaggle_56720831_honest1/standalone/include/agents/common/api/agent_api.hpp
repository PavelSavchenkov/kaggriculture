#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string_view>

#include "agents/common/api/observation.hpp"

namespace kag::agent {

struct AgentInfo {
    std::string_view name;
    uint32_t format_version = API_FORMAT_VERSION;
};

struct AgentInit {
    AgentConfig config;
    uint8_t player = 0;
};

struct DecisionBudget {
    using Clock = std::chrono::steady_clock;

    Clock::time_point soft_deadline = Clock::time_point::max();
    Clock::time_point hard_deadline = Clock::time_point::max();
    uint64_t max_expansions = std::numeric_limits<uint64_t>::max();

    bool soft_expired() const { return Clock::now() >= soft_deadline; }
    bool hard_expired() const { return Clock::now() >= hard_deadline; }
};

template<class T>
concept LocalAgent = std::default_initializable<T> && requires(
    T& policy,
    const AgentInit& init,
    const AgentObservation& observation,
    const DecisionBudget& budget,
    Action& action
) {
    { T::info() } -> std::same_as<AgentInfo>;
    { policy.reset(init) } -> std::same_as<void>;
    { policy.act(observation, budget, action) } -> std::same_as<void>;
};

}
