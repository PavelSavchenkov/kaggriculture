#include "cold_cases.hpp"
#include "solve_contract.hpp"
#include "schedule_bank.hpp"
#include "shop_scenario.hpp"
#include "life_route_repair.hpp"
#include "fixed_finance.hpp"
#include "finance_snapshot/compile_calendar.hpp"
#include "finance_snapshot/immediate_sales.hpp"
#include "agents/external/public_router/source/agent.hpp"
#include "replay_trace.hpp"
#include <iomanip>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc < 8 || (argc - 8) % 2) throw std::runtime_error("usage: compile_cold case new_output row|nearest|animals|assignment_file day_seconds query_seconds seed weed_chance [--bank manifest] [--opponent pass|public_router] [--seat 0|1] [--finance dawn|immediate_goods]");
        ScheduleBank bank; std::string opponent_name = "pass", finance = "dawn"; int seat = 0; bool reuse = false;
        bool repair_preparation = false;
        uint32_t refine_mask = (uint32_t{1} << 30) - 1;
        std::optional<uint64_t> shop_seed;
        std::optional<LifeRouteSource> route_source;
        std::optional<Course> finance_source;
        fs::path finance_source_path;
        std::optional<Trace> opponent_trace;
        fs::path opponent_trace_path;
        std::optional<double> warm_seconds;
        fs::path alternate_path; int decision_day = -1, milk_threshold = -1;
        const auto process_began = std::chrono::steady_clock::now();
        for (int arg = 8; arg < argc; arg += 2) {
            const std::string option(argv[arg]), value(argv[arg + 1]);
            if (option == "--bank") { bank.load(value); reuse = true; }
            else if (option == "--opponent") opponent_name = value;
            else if (option == "--opponent_trace") { opponent_trace.emplace(load(value)); validate(*opponent_trace); opponent_trace_path = fs::absolute(value); }
            else if (option == "--seat") seat = std::stoi(value);
            else if (option == "--finance") finance = value;
            else if (option == "--repair_preparation" && (value == "0" || value == "1")) repair_preparation = value == "1";
            else if (option == "--reuse" && (value == "none" || value == "self")) reuse = value == "self";
            else if (option == "--refine") refine_mask = std::stoul(value);
            else if (option == "--shop_seed") shop_seed = std::stoull(value);
            else if (option == "--route_source") route_source.emplace(value);
            else if (option == "--finance_source") { finance_source.emplace(value); finance_source_path = fs::absolute(value); }
            else if (option == "--warm_seconds") { warm_seconds = std::stod(value); if (*warm_seconds < 0) throw std::runtime_error("negative warm budget"); }
            else if (option == "--alternate") alternate_path = value;
            else if (option == "--decision_day") decision_day = std::stoi(value);
            else if (option == "--milk_threshold") milk_threshold = std::stoi(value);
            else throw std::runtime_error("unknown cold compiler option");
        }
        if ((opponent_name != "pass" && opponent_name != "public_router" && opponent_name != "recorded") || (finance != "dawn" && finance != "immediate_goods") || seat < 0 || seat > 1)
            throw std::runtime_error("invalid cold compiler option");
        if ((opponent_name == "recorded") != bool(opponent_trace)) throw std::runtime_error("recorded opponent needs exactly one source trace");
        const auto program = load_placement_program(argv[1]); const auto& lives = program.lives;
        if (route_source) route_source->validate(program);
        if (finance_source && finance != "dawn") throw std::runtime_error("fixed finance cannot also use immediate sales");
        if (finance_source) for (int d = 0; d < 30; ++d) {
            int purchases = 0;
            for (const auto& event : finance_source->days[d].problem.market_plan) if (event.market_op == kag::M_BUY_LAND) purchases += event.quantity;
            if (purchases != program.land[d]) throw std::runtime_error("fixed finance has a different land calendar");
        }
        const fs::path output(argv[2]); const std::string method(argv[3]);
        auto assignment = method == "row" || method == "nearest" || method == "animals" ?
            assign_lives(lives, method == "animals", method != "row") : read_life_assignment(method, lives.size());
        if (!legal_lives(lives, assignment) || fs::exists(output)) throw std::runtime_error("invalid assignment or output exists");
        LifeAssignment alternate;
        if (!alternate_path.empty()) {
            if (decision_day < 0 || decision_day >= 30 || milk_threshold < 0) throw std::runtime_error("invalid conditional placement decision");
            alternate = align_lives(lives, assignment, read_life_assignment(alternate_path, lives.size()));
            if (!legal_lives(lives, alternate)) throw std::runtime_error("illegal conditional placement");
            for (size_t i = 0; i < lives.size(); ++i) if (lives[i].spec.begin_day < decision_day && assignment[i] != alternate[i])
                throw std::runtime_error("conditional placement changes an earlier life");
        } else if (decision_day != -1 || milk_threshold != -1) throw std::runtime_error("conditional placement has no alternative");
        fs::create_directories(output); save_life_assignment(lives, assignment, output);
        if (finance_source) { std::ofstream saved(output / "finance_source.txt"); saved << finance_source_path.string() << '\n'; }
        save_placement_program(program, output / "INPUT.plan");
        kag::Config config; config.seed = std::stoull(argv[6]); config.weed_chance = std::stod(argv[7]);
        if (opponent_trace) {
            if (config.seed != opponent_trace->config.seed || config.weed_chance != opponent_trace->config.weed_chance) throw std::runtime_error("recorded-context seed/weed mismatch");
            config = opponent_trace->config;
        }
        kag::Sim sim(config);
        ShopScenario shops(shop_seed); std::ofstream shop_log(output / "shops.txt");
        std::ofstream decisions(output / "decisions.csv"); decisions << "day,milk_demand,threshold,selected\n";
        kag::agents::public_router::Agent opponent;
        opponent.reset(kag::agent::runtime::make_agent_init(sim, seat ^ 1));
        const kag::agent::DecisionBudget opponent_budget;
        const auto began = std::chrono::steady_clock::now();
        int64_t bill = 0; int queries = 0, complete_days = 0, clearing = 0; std::string failure;
        std::ofstream report(output / "days.csv"); report << "day,workers,bill,seconds,queries,cash,clearing,valid\n";
        std::array<int64_t, kag::N_ITEMS> expected_output{};
        for (int d = 0; d < 30; ++d) {
            shops.apply(sim);
            if (!alternate.empty() && d == decision_day) {
                const auto obs = kag::agent::runtime::make_observation(sim, seat); int demand = 0;
                for (int i = 0; i < obs.n_shops; ++i) if (kag::SHOP_MASK[obs.shops[i]] & (1u << kag::MILK)) demand += kag::SHOP_MULT[obs.shops[i]];
                const bool selected = demand >= milk_threshold;
                if (selected) { assignment = alternate; save_life_assignment(lives, assignment, output); }
                decisions << d << ',' << demand << ',' << milk_threshold << ',' << selected << '\n';
            }
            LifeDayContract contract;
            try {
                contract = compile_life_day(lives, assignment, d, sim.st.farms[seat], program.land[d], false);
                if (finance_source) apply_fixed_finance(contract, finance_source->days[d], d);
            } catch (const ResourceShortfall& error) {
                failure = error.what(); break;
            }
            for (int item = 0; item < kag::N_ITEMS; ++item) expected_output[item] += contract.output[item];
            auto warm = reuse ? bank.find(contract.day, d, 40, repair_preparation) : Certificate{};
            if (route_source) {
                auto repaired = route_source->find(contract.day, d, assignment, warm.schedule ? warm.workers - 1 : 40);
                if (repaired.schedule) warm = std::move(repaired);
            }
            const double seconds = !warm.schedule ? std::stod(argv[4]) : ((refine_mask >> d) & 1) ? std::min(std::stod(argv[4]), warm_seconds.value_or(std::stod(argv[4]))) : 0;
            auto certificate = solve_contract(contract.day, d, seconds, std::stod(argv[5]), warm.schedule ? &warm : nullptr);
            queries += certificate.queries; clearing += contract.clearing_actions;
            const auto folder = output / day_name(d); fs::create_directories(folder);
            day_solver::save_problem_json(certificate.problem ? *certificate.problem : contract.day.problem, folder / "problem.json");
            if (!certificate.schedule) { failure = "unknown_day_" + std::to_string(d); break; }
            if (reuse) bank.add(*certificate.problem, *certificate.schedule, folder.string());
            auto actions = executable(contract.day, *certificate.problem, *certificate.schedule);
            Schedule rival_actions;
            for (auto& action : rival_actions) { action.n_units = 1; action.finalize(); }
            labor::offline::save_actions(*certificate.schedule, folder / "physical.actions.txt");
            labor::offline::save_actions(actions, folder / "executable.actions.txt");
            bool valid = true;
            for (int h = 0; h < (d == 29 ? 23 : 24); ++h) {
                shops.apply(sim); save_shops(shop_log, sim, d * 24 + h);
                kag::Action pair[2]; pair[seat] = actions[h]; pair[seat ^ 1].n_units = sim.st.farms[seat ^ 1].n_units;
                if (opponent_name == "public_router") opponent.act(kag::agent::runtime::make_observation(sim, seat ^ 1), opponent_budget, pair[seat ^ 1]);
                if (opponent_trace) pair[seat ^ 1] = opponent_trace->turns[d * 24 + h].a[seat ^ 1];
                pair[seat ^ 1].finalize();
                rival_actions[h] = pair[seat ^ 1];
                if (finance == "immediate_goods") {
                    const auto obs = kag::agent::runtime::make_observation(sim, seat);
                    sales_planner::Account account; sales_planner::Resources resources;
                    if (!sales_planner::project_resources(obs, pair[seat], account, resources)) throw std::runtime_error("own resource projection failed");
                    sales_planner::Orders original; original.count = pair[seat].n_orders; std::copy_n(pair[seat].orders, original.count, original.values.begin());
                    std::array<int, kag::N_PRODUCTS> inventory; std::copy_n(obs.market.inventory, kag::N_PRODUCTS, inventory.begin());
                    const auto orders = sales_planner::immediate_sales(account, inventory, original, {}, false);
                    pair[seat].n_orders = orders.count; std::copy_n(orders.values.begin(), orders.count, pair[seat].orders); pair[seat].finalize();
                    actions[h] = pair[seat];
                }
                const auto diagnosis = sim.diagnose_joint_actions(pair[0], pair[1]).players[seat];
                if (diagnosis.requested_unit_actions != diagnosis.successful_unit_actions || diagnosis.requested_order_units != diagnosis.successful_order_units) {
                    valid = false; failure = "failed_action_day_" + std::to_string(d) + "_hour_" + std::to_string(h); break;
                }
                sim.step(pair[0], pair[1]);
                shops.advance();
            }
            labor::offline::save_actions(actions, folder / "executable.actions.txt");
            labor::offline::save_actions(rival_actions, folder / "opponent.actions.txt");
            for (int item = 0; item < kag::N_ITEMS; ++item) if (sim.st.farms[seat].produced[item] != expected_output[item]) {
                valid = false; if (failure.empty()) failure = "output_mismatch_day_" + std::to_string(d);
            }
            report << d << ',' << certificate.workers << ',' << labor::hire_cost(certificate.workers) << ',' << certificate.seconds << ','
                   << certificate.queries << ',' << sim.st.farms[seat].money << ',' << contract.clearing_actions << ',' << valid << '\n'; report.flush();
            if (!valid) break;
            bill += labor::hire_cost(certificate.workers); ++complete_days;
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"complete\":" << (complete_days == 30 ? "true" : "false") << ",\"days\":" << complete_days << ",\"hire_bill\":" << bill
                << ",\"seed\":" << config.seed << ",\"weed_chance\":" << config.weed_chance
                << ",\"cash\":" << sim.st.farms[seat].money << ",\"opponent_cash\":" << sim.st.farms[seat ^ 1].money << ",\"seat\":" << seat
                << ",\"queries\":" << queries << ",\"seconds\":" << seconds << ",\"process_seconds\":" << std::chrono::duration<double>(std::chrono::steady_clock::now() - process_began).count()
                << ",\"clearing_actions\":" << clearing << ",\"bank_entries\":" << bank.loaded << ",\"bank_checks\":" << bank.checks << ",\"bank_matches\":" << bank.matches
                << ",\"bank_weed_repairs\":" << bank.weed_repairs
                << ",\"bank_preparation_repairs\":" << bank.preparation_repairs << ",\"preparation_repair_enabled\":" << (repair_preparation ? "true" : "false")
                << ",\"route_repair_attempts\":" << (route_source ? route_source->attempts : 0) << ",\"route_repair_matches\":" << (route_source ? route_source->matches : 0)
                << ",\"reuse_enabled\":" << (reuse ? "true" : "false")
                << ",\"shop_seed\":" << (shop_seed ? std::to_string(*shop_seed) : "null")
                << ",\"failure\":\"" << failure << "\",\"opponent\":\"" << (opponent_trace ? "" : "live_") << opponent_name << "\",\"finance\":\"" << finance
                << "\",\"conditional_placement\":" << (!alternate.empty() ? "true" : "false")
                << ",\"placement_information\":\"own_state_currently_revealed_shops_and_fixed_lives\",\"finance_source\":";
        if (finance_source) summary << std::quoted(finance_source_path.string()); else summary << "null";
        summary << ",\"opponent_trace\":";
        if (opponent_trace) summary << std::quoted(opponent_trace_path.string()); else summary << "null";
        summary << "}\n";
        std::cout << complete_days << "/30 days; bill " << bill << "; " << failure << '\n';
        return complete_days == 30 ? 0 : 1;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
