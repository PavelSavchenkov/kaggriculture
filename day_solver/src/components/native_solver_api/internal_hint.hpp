#pragma once

#include <array>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace day_native {
// These describe OUR intermediate proposals. The public solver accepts v3 only.
struct HintAssignment {
    int task, worker;
    std::optional<int> hour, type, chain;
};
struct InternalHint {
    std::vector<HintAssignment> assignments;
    std::optional<std::vector<std::vector<int>>> type_workers;
    bool partial_routes = false;
};
enum class HintKind { partial, fixed_partial, route_task, route_type, route_owner, chain_type };
enum class HintFlag { fix_partial, diagnose_partial, diagnose_task_route, diagnose_type_route, diagnose_owner };
using CoreIdentity = std::variant<int, std::array<int, 2>>;

inline const char* hint_name(HintKind kind) {
    constexpr std::array names{"--partial-hint", "--fixed-partial-hint", "--fixed-route-task-hint",
        "--fixed-route-type-hint", "--fixed-route-owner-type-hint", "--fixed-chain-type-hint"};
    return names.at(int(kind));
}
inline const char* flag_name(HintFlag flag) {
    constexpr std::array names{"--fix-partial-hint", "--diagnose-partial-core", "--diagnose-route-task-core",
        "--diagnose-route-type-core", "--diagnose-task-owner-core"};
    return names.at(int(flag));
}
template<class T> const T& required(const std::optional<T>& value, const char* field) {
    if (!value) throw std::runtime_error(std::string("missing internal hint field: ") + field);
    return *value;
}
} // namespace day_native
