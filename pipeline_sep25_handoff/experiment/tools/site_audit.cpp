// Site-capacity audit: for one perspective-day, prints every tile that holds a crop or
// animal established today (at the next dawn) with its state at this dawn.
// usage: site_audit trace seat day
#include "source/convert.hpp"
#include <cstdio>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 4) return 2;
    const Replay replay = load_replay(argv[1]);
    const auto states = replay_states(replay);
    const int seat = std::atoi(argv[2]), day = std::atoi(argv[3]);
    const auto& dawn = states[day * HOURS].st.farms[seat];
    const auto& next = states[(day + 1) * HOURS].st.farms[seat];
    const char* kinds[] = {"empty", "locked", "weed", "coop", "pasture", "plant"};
    for (int c = 0; c < BOARD * BOARD; ++c) {
        const Tile& a = dawn.tiles[c / BOARD][c % BOARD];
        const Tile& b = next.tiles[c / BOARD][c % BOARD];
        const bool established = (b.kind == T_PLANT || b.has_animal) && b.planted_day == day;
        if (!established) continue;
        std::printf("cell %2d new %s%d | dawn %s what %d animal %d age %d yield %d dry %d expiry %d (day end step %d)\n", c,
                    b.has_animal ? "animal" : "crop", b.what, kinds[a.kind], a.what, a.has_animal, day - a.planted_day,
                    a.yield_units, a.consecutive_dry, a.max_lifespan_step, (day + 1) * HOURS);
    }
    const Conversion conv = convert_day(states, replay.turns, seat, day);
    std::printf("label: %s\n", conv.failure.c_str());
}
