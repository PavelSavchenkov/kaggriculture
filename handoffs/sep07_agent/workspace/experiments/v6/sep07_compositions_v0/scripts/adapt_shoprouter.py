"""Offline source adapter; preserves the original policy in policy.txt."""
from pathlib import Path


EXPERIMENT = Path(__file__).resolve().parents[1]
UPSTREAM = EXPERIMENT / "league/teammate_shoprouter/source/upstream"
source = (UPSTREAM / "policy.txt").read_text()
source = source[:source.index('extern "C"')]
source = source.replace('#include "policy_plugin_abi.hpp"', '#include "runtime_types.hpp"')
source = source.replace('#include <stdexcept>', '#include <cstdlib>')
source = source.replace('throw std::runtime_error("invalid encoded tape action counts");', 'std::abort();')
source = source.replace('throw std::runtime_error("invalid encoded unit action");', 'std::abort();')
source = source.replace('throw std::runtime_error("invalid encoded market order");', 'std::abort();')
source = source.replace('throw std::runtime_error("trailing encoded tape value");', 'std::abort();')
start = source.index('    std::array<std::array<kag::Action, kTurns>, kRoutes> actions{};')
end = source.index('    kag::Action action_for', start)
source = source[:start] + '''    int selected_route = 0;
    static const auto& decoded_actions() {
        static const auto actions = [] {
            std::array<std::array<kag::Action, kTurns>, kRoutes> result{};
            for (int route = 0; route < kRoutes; ++route)
                for (int step = 0; step < kTurns; ++step)
                    result[route][step] = decode_action(kEncodedTapes[route][step]);
            return result;
        }();
        return actions;
    }

''' + source[end:]
source = source.replace('return actions[selected_route]', 'return decoded_actions()[selected_route]')
(UPSTREAM / "policy.hpp").write_text(source)
