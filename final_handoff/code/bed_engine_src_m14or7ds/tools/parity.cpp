// Native inference parity: global-head logits for validation dawns, printed for
// comparison with scripts/parity.py on the extracted arrays.
// usage: parity corpus.txt count > native.txt
#include "agent/bc_opus/source/agent.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    std::ifstream corpus(argv[1]);
    const int count = std::atoi(argv[2]);
    kag::agents::bc_opus::Model model;
    if (!model.load(std::getenv("BC_OPUS_MODEL"))) return 1;
    std::string line;
    int done = 0;
    while (done < count && std::getline(corpus, line)) {
        std::istringstream words(line);
        long episode;
        int seat;
        std::string team, submission, split, path;
        words >> episode >> seat >> team >> submission >> split >> path;
        if (split != "validation") continue;
        const Replay replay = load_replay(path);
        const auto states = replay_states(replay);
        History history;
        for (int day = 0; day <= LAST_DAY && done < count; day += 7) {
            for (int k = (day ? day - 7 : 0) * HOURS; k < day * HOURS; ++k)
                history.observe(agent::runtime::make_observation(states[k], seat), replay.turns[k][seat]);
            std::vector<float> logits;
            kag::agents::bc_opus::decode_intent(model, agent::runtime::make_observation(states[day * HOURS], seat), history,
                                                nullptr, &logits);
            std::cout << episode << ' ' << seat << ' ' << day;
            for (float v : logits) std::cout << ' ' << v;
            std::cout << '\n';
            ++done;
        }
    }
}
