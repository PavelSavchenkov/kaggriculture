#include "forecast.hpp"
#include "registry.hpp"
#include "../../include/market_tape.hpp"
#include "../../include/crop_branch_sequence.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>

using namespace compositions;
using Course = CropBranchSequence<crop_value_m2_t4::Agent>;

void write_days(std::ostream& out, const crop_choice::Days& days) {
    out << "{{";
    for (int day = 0; day < 30; ++day) {
        if (day) out << ',';
        out << "{{";
        for (int item = 0; item < 9; ++item) out << (item ? "," : "") << days[day][item];
        out << "}}";
    }
    out << "}}";
}

int main(int argc, char** argv) {
    if (argc != 2 || std::filesystem::exists(argv[1])) return 2;
    const std::filesystem::path output(argv[1]);
    std::filesystem::create_directories(output);
    std::ofstream header(output / "plans.hpp"), audit(output / "audit.csv");
    header << "#pragma once\n// Exact accepted own flows from four complete courses; see audit.csv.\n"
              "#include \"../forecast.hpp\"\nnamespace compositions::crop_choice {\n"
              "inline const std::array<Plan,4> financial_plans={{\n";
    audit << "family,berry,own_cash,rival_cash,revalued_own,revalued_rival,matched_days,own_revenue,own_spend\n";
    for (int family = 0; family < 2; ++family) for (int berry = 0; berry < 2; ++berry) {
        auto off = family ? crop_rotation_t2_berry::off_days() : wheat_one_fert::off_days();
        auto on = family ? crop_rotation_t2_berry::on_days() : wheat_one_fert::on_days();
        Course own(std::move(off), std::move(on), 20, berry ? 0 : 100000);
        auto rival = make_agent("public_router");
        kag::Config config; config.seed = 1000;
        kag::Sim sim(config);
        own.reset(kag::agent::runtime::make_agent_init(sim, 0));
        rival.reset(kag::agent::runtime::make_agent_init(sim, 1));
        uint64_t random = config.seed ^ 0xa37108e62d045fb9ULL;
        std::array<uint8_t,8> shops{};
        for (auto& shop : shops) shop = random_word(random) % kag::N_SHOPS;
        MarketTape tape{};
        crop_choice::Plan plan{};
        while (!sim.st.done) {
            std::copy_n(shops.begin(), sim.st.n_shops, sim.st.shops);
            kag::Action actions[2];
            const auto observation = kag::agent::runtime::make_observation(sim, 0);
            own.act(observation, {}, actions[0]);
            rival.act(kag::agent::runtime::make_observation(sim, 1), {}, actions[1]);
            validate_action(actions[0], observation);
            auto& step = tape[sim.st.step];
            step = accepted_market(sim, actions);
            for (const auto& slot : step.slots) {
                plan.fixed[sim.st.day] += slot.fixed[0];
                const auto& trade = slot.trades[0];
                if (trade.op == kag::M_SELL) plan.sales[sim.st.day][trade.item] += trade.n;
                if (trade.op == kag::M_BUY_PRODUCT) plan.buys[sim.st.day][trade.item] += trade.n;
            }
            sim.step(actions[0], actions[1]);
        }
        uint32_t expected = 0;
        for (int day = family ? 12 : 13; day <= 28; ++day) expected |= uint32_t(1) << day;
        if (own.matched_days() != expected || own.berry_selected() != bool(berry)) std::abort();
        const auto exact = value_market(tape, shops);
        for (int seat = 0; seat < 2; ++seat)
            if (exact.cash[seat] != sim.st.farms[seat].money) std::abort();
        header << "Plan{";
        write_days(header, plan.sales); header << ',';
        write_days(header, plan.buys); header << ",{{";
        for (int day = 0; day < 30; ++day) header << (day ? "," : "") << plan.fixed[day];
        header << "}}},\n";
        audit << family << ',' << berry << ',' << sim.st.farms[0].money << ',' << sim.st.farms[1].money << ','
              << exact.cash[0] << ',' << exact.cash[1] << ',' << own.matched_days() << ','
              << sim.st.farms[0].sell_revenue << ',' << sim.st.farms[0].total_spend << '\n';
        std::cout << "family " << family << " berry " << berry << ": full guard course and both cash values exact\n";
    }
    header << "}};\n}\n";
}
