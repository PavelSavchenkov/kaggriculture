// Melon sale race: for each trace, per seat and day, the first hour melons were sold, units
// sold and the mean market price over those sales (price before each step, which the first
// unit of that step gets). Melons have almost no shop demand, so the first seller of a harvest
// wave takes the price.
// usage: melon_race list.txt out.csv   (list line: trace)
#include "source/world.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: melon_race list.txt out.csv\n";
        return 2;
    }
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    out << "trace,seat,day,first_hour,units,mean_price\n";
    for (std::string trace; std::getline(list, trace);) {
        if (trace.empty()) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int first[2] = {-1, -1}, units[2] = {0, 0};
        double value[2] = {0, 0};
        auto flush = [&](int day) {
            for (int seat = 0; seat < 2; ++seat)
                if (units[seat])
                    out << trace << ',' << seat << ',' << day << ',' << first[seat] << ',' << units[seat] << ','
                        << value[seat] / units[seat] << '\n';
            first[0] = first[1] = -1;
            units[0] = units[1] = 0;
            value[0] = value[1] = 0;
        };
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const int hour = int(s % HOURS);
            if (hour == 0 && s) flush(int(s / HOURS) - 1);
            const double price = sim.st.market.prices[MELON];
            const int32_t before[2] = {sim.st.farms[0].sold_units[MELON], sim.st.farms[1].sold_units[MELON]};
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            for (int seat = 0; seat < 2; ++seat) {
                const int n = sim.st.farms[seat].sold_units[MELON] - before[seat];
                if (n <= 0) continue;
                if (first[seat] < 0) first[seat] = hour;
                units[seat] += n;
                value[seat] += price * n;
            }
        }
        flush(int(replay.turns.size() / HOURS));
    }
    return 0;
}
