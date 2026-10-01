// Teacher-forced decoding quality: decode our DayIntent on replay dawns (replay state
// and causal history) and compare each field with the converted replay label.
// Fixed values are excluded by construction (both sides equal them).
// usage: decode_eval corpus.txt out.csv     (BC_OPUS_MODEL, BC_COUNT_MODE as in the agent)
#include "agent/bc_opus/source/agent.hpp"
#include "source/convert.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    std::ifstream corpus(argv[1]);
    std::ofstream out(argv[2]);
    out << "episode,seat,day,new_crop_pred,new_crop_label,new_crop_abs,new_animal_pred,new_animal_label,new_animal_abs,"
           "reserve_abs,land_pred,land_label,oneshot_members,oneshot_mismatch,harvest_pred,harvest_label,retain_pred,"
           "retain_label,clear_pred,clear_label,feed_pred,feed_label,care_pred,care_label,collect_pred,collect_label,"
           "water_pred,water_label,fert_pred,fert_label,distance\n";
    kag::agents::bc_opus::Model model;
    if (!model.load(std::getenv("BC_OPUS_MODEL"))) return 1;
    std::string line;
    while (std::getline(corpus, line)) {
        std::istringstream words(line);
        long episode;
        int seat;
        std::string team, submission, split, path;
        words >> episode >> seat >> team >> submission >> split >> path;
        const Replay replay = load_replay(path);
        const auto states = replay_states(replay);
        History history;
        int fed = 0;
        for (int day = 0; day <= LAST_DAY; ++day) {
            for (; fed < day * HOURS; ++fed)
                history.observe(agent::runtime::make_observation(states[fed], seat), replay.turns[fed][seat]);
            const Conversion label = convert_day(states, replay.turns, seat, day);
            if (label.status != ConvertStatus::Ok) continue;
            const auto dawn = agent::runtime::make_observation(states[day * HOURS], seat);
            const DayIntent pred = kag::agents::bc_opus::decode_intent(model, dawn, history);
            // Compare on the label's schema for existing groups; new groups by counts.
            Schema s = label.schema;
            const DayIntent& l = label.intent;
            int nc[2]{}, na[2]{}, nca = 0, naa = 0, ra = 0, members = 0, mismatch = 0;
            int harvest[2]{}, retain[2]{}, clear[2]{}, feed[2]{}, care[2]{}, collect[2]{}, water[2]{}, fert[2]{};
            for (int c = 0; c < N_CROPS; ++c) nc[0] += pred.new_crop[c], nc[1] += l.new_crop[c], nca += std::abs(pred.new_crop[c] - l.new_crop[c]);
            for (int a = 0; a < N_ANIMALS; ++a) {
                na[0] += pred.new_animal[a], na[1] += l.new_animal[a], naa += std::abs(pred.new_animal[a] - l.new_animal[a]);
                ra += std::abs(pred.reserve[a] - l.reserve[a]);
            }
            for (int i = 0; i < s.n_crops; ++i) {
                const CropGroup& g = s.crops[i];
                const DayIntent* both[2] = {&pred, &l};
                for (int k = 0; k < 2; ++k) {
                    const DayIntent& in = *both[k];
                    for (int o = 0; o < OPTIONS; ++o) {
                        if (g.fresh) continue;
                        harvest[k] += has_harvest(o) * in.options[i][o];
                        water[k] += has_water(o) * in.options[i][o];
                        fert[k] += has_fertilize(o) * in.options[i][o];
                        clear[k] += (o == CLEAR) * in.options[i][o];
                    }
                    if (!g.fresh) harvest[k] += in.harvest[i], retain[k] += in.retain[i], clear[k] += in.clear[i], fert[k] += in.fertilize[i];
                }
                if (!g.fresh && !CROPS[g.crop].ongoing) {
                    members += g.size;
                    int diff = 0;
                    for (int o = 0; o < OPTIONS; ++o) diff += std::abs(pred.options[i][o] - l.options[i][o]);
                    mismatch += diff / 2;
                }
            }
            for (int i = 0; i < s.n_animals; ++i) {
                if (s.animals[i].fresh) continue;
                feed[0] += pred.feed[i], feed[1] += l.feed[i];
                care[0] += pred.care[i], care[1] += l.care[i];
                collect[0] += pred.collect[i], collect[1] += l.collect[i];
            }
            int distance = nca + naa + ra + (pred.buy_land != l.buy_land) + mismatch * 2;
            distance += std::abs(harvest[0] - harvest[1]);
            out << episode << ',' << seat << ',' << day << ',' << nc[0] << ',' << nc[1] << ',' << nca << ',' << na[0] << ','
                << na[1] << ',' << naa << ',' << ra << ',' << pred.buy_land << ',' << l.buy_land << ',' << members << ','
                << mismatch << ',' << harvest[0] << ',' << harvest[1] << ',' << retain[0] << ',' << retain[1] << ','
                << clear[0] << ',' << clear[1] << ',' << feed[0] << ',' << feed[1] << ',' << care[0] << ',' << care[1] << ','
                << collect[0] << ',' << collect[1] << ',' << water[0] << ',' << water[1] << ',' << fert[0] << ',' << fert[1]
                << ',' << distance << '\n';
        }
    }
    return 0;
}
