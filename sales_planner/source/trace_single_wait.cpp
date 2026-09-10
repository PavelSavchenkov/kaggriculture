#include "case.hpp"
#include "timing.hpp"
#include "physical_state.hpp"

using namespace sales_planner;

int main(int argc, char** argv) {
    require(argc == 7, "usage: trace_single_wait case seat from to item quantity");
    const auto c = read_case(argv[1]);
    const int seat = std::stoi(argv[2]), from = std::stoi(argv[3]), to = std::stoi(argv[4]);
    const int item = std::stoi(argv[5]), quantity = std::stoi(argv[6]);
    require(seat >= 0 && seat < 2 && from >= 0 && from < to && to < int(c.turns.size()), "bad edit period");
    kag::Sim reference(c.config), candidate(c.config);
    double last_gain = 0;
    std::array<int, kag::N_CROPS> last_seeds{};
    int changed_turns = 0;
    for (int t = 0; t < int(c.turns.size()); ++t) {
        const auto& source = c.turns[t];
        auto actions = source.original_actions;
        auto orders = source.original_orders[seat];
        if (t == from) {
            int removed = 0;
            for (int k = 0; k < orders.count; ++k) {
                auto& o = orders.values[k];
                if (o.op == kag::M_SELL && o.item == item) {
                    require(!removed && o.n == quantity, "expected one exact source sale");
                    o = {}; removed = quantity;
                }
            }
            require(removed == quantity, "source sale missing");
        }
        if (t == to) require(add_sale(orders, item, quantity, c.config.max_orders), "target full");
        actions[seat].n_orders = orders.count;
        std::copy_n(orders.values.begin(), orders.count, actions[seat].orders);
        actions[seat].finalize();
        reference.step(source.original_actions[0], source.original_actions[1]);
        candidate.step(actions[0], actions[1]);
        const auto& a = reference.st.farms[seat]; const auto& b = candidate.st.farms[seat];
        require(a.money == source.expected.financial.accounts[seat].cash, "source parity failed");
        const double gain = b.money - a.money;
        std::array<int, kag::N_CROPS> seeds;
        for (int i = 0; i < kag::N_CROPS; ++i) seeds[i] = b.seeds[i] - a.seeds[i];
        const int physical = physical_difference(a, b);
        changed_turns += physical != 0;
        if (gain != last_gain || seeds != last_seeds || t + 1 == int(c.turns.size())) {
            std::printf("{\"turn\":%d,\"reference_cash\":%.0f,\"candidate_cash\":%.0f,\"cash_gain\":%.0f,\"physical_mask\":%d,\"seed_difference\":[", t, a.money, b.money, gain, physical);
            for (int i = 0; i < kag::N_CROPS; ++i) std::printf("%s%d", i ? "," : "", seeds[i]);
            std::printf("],\"orders\":[");
            for (int k = 0; k < orders.count; ++k) {
                const auto o = orders.values[k];
                std::printf("%s[%d,%d,%d]", k ? "," : "", o.op, o.item, o.n);
            }
            std::printf("]}\n");
        }
        last_gain = gain; last_seeds = seeds;
    }
    std::fprintf(stderr, "{\"physical_changed_turns\":%d}\n", changed_turns);
}
