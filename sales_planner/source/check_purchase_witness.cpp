#include "case.hpp"
#include "physical_state.hpp"

using namespace sales_planner;

int main(int argc, char** argv) {
    require(argc > 1, "usage: check_purchase_witness case.calendar [...]");
    for (int file = 1; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        int checked = 0, changed_orders = 0;
        for (int seat = 0; seat < 2; ++seat) {
            kag::Sim reference(c.config), candidate(c.config);
            for (const auto& turn : c.turns) {
                auto actions = turn.original_actions;
                const auto& orders = turn.purchase_witness[seat];
                for (int k = 0; k < orders.count; ++k) {
                    const auto a = orders.values[k], b = turn.original_orders[seat].values[k];
                    changed_orders += a.op != b.op || a.item != b.item || a.n != b.n;
                }
                actions[seat].n_orders = orders.count;
                std::copy_n(orders.values.begin(), orders.count, actions[seat].orders);
                actions[seat].finalize();
                reference.step(turn.original_actions[0], turn.original_actions[1]);
                candidate.step(actions[0], actions[1]);
                for (int p = 0; p < 2; ++p) {
                    const auto& a = reference.st.farms[p]; const auto& b = candidate.st.farms[p];
                    require(!physical_difference(a, b), "purchase normalization changes physical state");
                    require(a.money == b.money, "purchase normalization changes cash");
                    require(std::equal(a.shed, a.shed + kag::N_ITEMS, b.shed), "purchase normalization changes shed");
                    require(std::equal(a.produced, a.produced + kag::N_ITEMS, b.produced), "purchase normalization changes output");
                    require(std::equal(a.discarded, a.discarded + kag::N_ITEMS, b.discarded), "purchase normalization changes discards");
                }
                require(std::equal(reference.st.market.inventory, reference.st.market.inventory + kag::N_PRODUCTS,
                    candidate.st.market.inventory), "purchase normalization changes market");
                require(reference.st.n_shops == candidate.st.n_shops &&
                    std::equal(reference.st.shops, reference.st.shops + reference.st.n_shops, candidate.st.shops),
                    "purchase normalization changes shops");
                ++checked;
            }
        }
        std::printf("{\"episode\":%llu,\"checked_turns\":%d,\"changed_order_requests\":%d}\n",
            (unsigned long long)c.episode, checked, changed_orders);
    }
}
