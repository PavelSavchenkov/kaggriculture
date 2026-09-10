#include "replay_trace.hpp"
#include <bit>

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: profile_trace trace seat output_directory");
        const auto trace = load(argv[1]); validate(trace);
        const int seat = std::stoi(argv[2]);
        if (seat < 0 || seat > 1) throw std::runtime_error("invalid seat");
        const fs::path output(argv[3]); fs::create_directories(output);
        std::ofstream workers(output / "workers.csv"), snapshots(output / "days.csv"), edges(output / "edges.csv");
        workers << "day,worker,field_actions,quadrants,crossings,q0,q1,q2,q3\n";
        snapshots << "day,cash,shops,milk_demand,wool_demand,milk_price,wool_price,cows,sheep,cow_radius,sheep_radius\n";
        edges << "day,worker,from_hour,to_hour,from_cell,to_cell,from_op,to_op\n";
        std::array<std::array<int, 4>, 40> counts{};
        std::array<int, 40> last_q, crossings{}, last_cell{}, last_hour{}, last_op{};
        last_q.fill(-1); last_cell.fill(-1); last_hour.fill(-1);
        Sim sim(trace.config);
        for (const auto& turn : trace.turns) {
            const auto& farm = sim.st.farms[seat];
            if (sim.st.hour == 0) {
                int demand[N_PRODUCTS]{};
                for (int s = 0; s < sim.st.n_shops; ++s) for (int p = 0; p < N_PRODUCTS; ++p)
                    if (SHOP_MASK[sim.st.shops[s]] & (1 << p)) demand[p] += SHOP_MULT[sim.st.shops[s]];
                int cows = 0, sheep = 0, cow_radius = 0, sheep_radius = 0;
                for (int cell = 0; cell < 100; ++cell) {
                    const auto& tile = farm.tiles[cell / 10][cell % 10];
                    const int x = cell % 10, y = cell / 10;
                    const int distance = std::min(abs(x - 4), abs(x - 5)) + std::min(abs(y - 4), abs(y - 5));
                    if (tile.has_animal && tile.what == COW) { ++cows; cow_radius += distance; }
                    if (tile.has_animal && tile.what == SHEEP) { ++sheep; sheep_radius += distance; }
                }
                snapshots << sim.st.day << ',' << farm.money << ',' << sim.st.n_shops << ',' << demand[MILK] << ',' << demand[WOOL]
                          << ',' << sim.st.market.prices[MILK] << ',' << sim.st.market.prices[WOOL] << ',' << cows << ',' << sheep << ','
                          << (cows ? double(cow_radius) / cows : -1) << ',' << (sheep ? double(sheep_radius) / sheep : -1) << '\n';
            }
            Action action = turn.a[seat]; action.n_orders = 0; action.finalize();
            const auto accepted = sim.sanitize_solo_action(seat, action);
            for (int u = 0; u < farm.n_units; ++u) {
                const auto& operation = accepted.units[u];
                const int cell = farm.pos_y[u] * 10 + farm.pos_x[u];
                const auto& tile = farm.tiles[cell / 10][cell % 10];
                const bool place = operation.op == OP_PLACE && is_animal(operation.arg) && !tile.has_animal &&
                    tile.kind == (operation.arg == GOOSE ? T_COOP : T_PASTURE);
                const bool field = place || (operation.op >= OP_PLANT && operation.op <= OP_CARE);
                if (field) {
                    const int q = quadrant_of(cell % 10, cell / 10, 10);
                    ++counts[u][q]; crossings[u] += last_q[u] >= 0 && last_q[u] != q; last_q[u] = q;
                }
                const bool anchor = field || operation.op == OP_PICKUP || operation.op == OP_DROP || operation.op == OP_PLACE;
                if (anchor) {
                    if (last_cell[u] >= 0) edges << sim.st.day << ',' << u << ',' << last_hour[u] << ',' << sim.st.hour << ','
                        << last_cell[u] << ',' << cell << ',' << last_op[u] << ',' << +operation.op << '\n';
                    last_cell[u] = cell; last_hour[u] = sim.st.hour; last_op[u] = operation.op;
                }
            }
            if (sim.st.hour == 23 || sim.st.step == 718) {
                for (int u = 0; u < farm.n_units; ++u) {
                    int tasks = 0, mask = 0;
                    for (int q = 0; q < 4; ++q) { tasks += counts[u][q]; if (counts[u][q]) mask |= 1 << q; }
                    if (tasks) workers << sim.st.day << ',' << u << ',' << tasks << ',' << std::popcount(unsigned(mask)) << ',' << crossings[u]
                        << ',' << counts[u][0] << ',' << counts[u][1] << ',' << counts[u][2] << ',' << counts[u][3] << '\n';
                }
                counts = {}; crossings = {}; last_q.fill(-1); last_cell.fill(-1); last_hour.fill(-1); last_op = {};
            }
            sim.step(turn.a[0], turn.a[1]);
        }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
