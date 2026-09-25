// Dataset generation and interface-coverage gate (designs/day_intent.md).
// For every perspective-day: convert the replay day to its DayIntent label, then write
// actor inputs (dawn observation, schema, causal opponent-flow history), targets and
// fixed-value masks. Ignored days are counted by reason; any failed conversion is
// written to <prefix>.failures.csv and makes the tool exit with status 1.
// usage: extract corpus.txt shard shards out_prefix
// corpus line: episode seat team_id submission split trace
#include "source/convert.hpp"
#include "source/dataset.hpp"
#include <iostream>
#include <map>

using namespace bcopus;

namespace {
template <class T>
void write(std::ofstream& out, const T* data, size_t count) {
    out.write(reinterpret_cast<const char*>(data), std::streamsize(sizeof(T) * count));
}
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: extract corpus.txt shard shards out_prefix\n";
        return 2;
    }
    const int shard = std::atoi(argv[2]), shards = std::atoi(argv[3]);
    const std::string prefix = argv[4];
    std::ifstream corpus(argv[1]);
    auto open = [&](const char* suffix) { return std::ofstream(prefix + suffix, std::ios::binary); };
    auto meta = open(".meta.i64"), glob = open(".global.f32"), gtarget = open(".gtarget.i16"), gmask = open(".gmask.u8");
    auto crop = open(".crop.f32"), ctarget = open(".ctarget.i16"), cmask = open(".cmask.u8");
    auto animal = open(".animal.f32"), atarget = open(".atarget.i16"), amask = open(".amask.u8");
    auto grid = open(".grid.f32");
    std::ofstream failures(prefix + ".failures.csv");
    failures << "episode,seat,day,failure\n";
    std::map<std::string, long> totals;
    long episode = 0, team = 0, submission = 0;
    int seat = 0, line = 0;
    std::string split, path;
    while (corpus >> episode >> seat >> team >> submission >> split >> path) {
        if (line++ % shards != shard) continue;
        const Replay replay = load_replay(path);
        const auto states = replay_states(replay);
        ++totals["perspectives"];
        History history;
        int fed = 0;
        for (int day = 0; day <= LAST_DAY; ++day) {
            const int start = day * HOURS;
            for (; fed < start; ++fed)
                history.observe(agent::runtime::make_observation(states[fed], seat), replay.turns[fed][seat]);
            const Conversion c = convert_day(states, replay.turns, seat, day);
            ++totals["days"];
            totals["dropped_actions"] += c.dropped_actions;
            if (c.status == ConvertStatus::IgnoredLand) { ++totals["ignored_multiple_land"]; continue; }
            if (c.status == ConvertStatus::IgnoredDiscard) { ++totals["ignored_discarded_animal"]; continue; }
            if (c.status == ConvertStatus::Failed) {
                ++totals["failed"];
                failures << episode << ',' << seat << ',' << day << ",\"" << c.failure << "\"\n";
                continue;
            }
            ++totals["represented"];
            const auto dawn = agent::runtime::make_observation(states[start], seat);
            // Actor inputs use the dawn schema; new-group sizes come from the label
            // (teacher forcing), exactly as the runtime sets them from decoded counts.
            const Schema& s = c.schema;
            double rival[HOURS][N_PRODUCTS];
            history.expected(day, 3, rival);
            float g[GLOBAL];
            global_features(dawn, s, rival, g);
            int16_t gt[GLOBAL_FIELDS];
            uint8_t gm[GLOBAL_FIELDS];
            global_targets(c.intent, gt);
            global_masks(s, gm);
            const int64_t m[8] = {episode, seat, day, team, submission, split == "train" ? 0 : split == "validation" ? 1 : 2,
                                  s.n_crops, s.n_animals};
            write(meta, m, 8);
            write(glob, g, GLOBAL);
            write(gtarget, gt, GLOBAL_FIELDS);
            write(gmask, gm, GLOBAL_FIELDS);
            static float cells[2][GRID_CHANNELS * BOARD * BOARD];
            grid_features(dawn.self(), day, cells[0]);
            grid_features(dawn.opponent(), day, cells[1]);
            write(grid, &cells[0][0], 2 * GRID_CHANNELS * BOARD * BOARD);
            for (int i = 0; i < s.n_crops; ++i) {
                const CropGroup& group = s.crops[i];
                float f[CROP];
                crop_features(s, group, f, dawn.market.prices[group.crop]);
                int16_t t[CROP_TARGETS];
                uint8_t k[CROP_MASKS];
                crop_targets(c.intent, i, group.size, t);
                crop_masks(s, group, group.size, k);
                // Fixed values must agree with the label (interface-coverage gate).
                bool conflict = false;
                for (int o = 0; o < OPTIONS; ++o) conflict |= k[o] && t[1 + o];
                if (CROPS[group.crop].ongoing && !group.fresh) {
                    const int abandon = group.size - t[10] - t[11];
                    conflict |= (k[9] && t[10]) || (k[10] && t[11]) || (k[12] && t[12]) || (k[13] && t[13]);
                    conflict |= day < LAST_DAY && k[11] && abandon;
                }
                if (conflict) {
                    ++totals["fixed_value_conflicts"];
                    failures << episode << ',' << seat << ',' << day << ",\"fixed-value conflict in crop group " << i << "\"\n";
                }
                write(crop, f, CROP);
                write(ctarget, t, CROP_TARGETS);
                write(cmask, k, CROP_MASKS);
            }
            for (int i = 0; i < s.n_animals; ++i) {
                const AnimalGroup& group = s.animals[i];
                float f[ANIMAL];
                animal_features(s, group, f, dawn.market.prices[ANIMALS[group.species].product]);
                int16_t t[ANIMAL_TARGETS];
                uint8_t k[ANIMAL_MASKS];
                animal_targets(c.intent, i, group.size, t);
                animal_masks(s, group, group.size, k);
                bool conflict = false;
                for (int j = 0; j < ANIMAL_MASKS; ++j) conflict |= k[j] && t[1 + j];
                if (conflict) {
                    ++totals["fixed_value_conflicts"];
                    failures << episode << ',' << seat << ',' << day << ",\"fixed-value conflict in animal group " << i << "\"\n";
                }
                write(animal, f, ANIMAL);
                write(atarget, t, ANIMAL_TARGETS);
                write(amask, k, ANIMAL_MASKS);
            }
        }
    }
    for (const auto& [name, value] : totals) std::cout << name << ',' << value << '\n';
    return totals["failed"] || totals["fixed_value_conflicts"] ? 1 : 0;
}
