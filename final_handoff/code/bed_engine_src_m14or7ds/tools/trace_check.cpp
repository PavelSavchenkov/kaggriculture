// Verifies traces against this engine (header, actions, per-step parity hashes).
// usage: trace_check trace... ; prints "ok <path>" or "fail <path> <reason>".
#include "source/world.hpp"
#include <iostream>

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        try {
            const auto replay = dc10::load_replay(argv[i]);
            dc10::replay_states(replay);
            std::cout << "ok " << argv[i] << '\n';
        } catch (const std::exception& e) {
            std::cout << "fail " << argv[i] << ' ' << e.what() << '\n';
        }
    }
    return 0;
}
