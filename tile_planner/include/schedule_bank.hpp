#pragma once
#include "certify.hpp"
#include "weed_schedule_repair.hpp"
#include "bank_binary.hpp"
#include "preparation_repair.hpp"
#include <unordered_set>

namespace placement {
// A lookup filter only. Quantities, ages and resources are deliberately not
// keys; every returned schedule is checked against the complete new contract.
inline std::string work_key(const DayProblem& p, bool strip_weed_clear = false) {
    std::vector<std::pair<int, const day_solver::TileWork*>> work;
    for (const auto& tile : p.tile_work) {
        const auto& cell = p.start.managed_tiles[tile.tile]; work.emplace_back(cell.y * 10 + cell.x, &tile);
    }
    std::sort(work.begin(), work.end());
    std::string key;
    for (const auto& [cell, tile] : work) {
        key.push_back(char(cell));
        for (size_t i = 0; i < tile->actions.size(); ++i) {
            const auto& a = tile->actions[i];
            if (strip_weed_clear && i == 0 && a.op == kag::OP_DIG && p.start.managed_tiles[tile->tile].state.kind == day_solver::ManagedTileKind::WEED) continue;
            key.push_back(char(a.op)); key.push_back(char(a.op == kag::OP_HARVEST ? 0 : a.arg + 1));
        }
        key.push_back(char(255));
    }
    return key;
}

class ScheduleBank {
    struct Entry { int workers; std::vector<day_solver::MarketEvent> hires; Schedule schedule; std::string source; };
    std::unordered_map<std::string, std::vector<Entry>> entries;
    std::unordered_map<std::string, std::vector<std::string>> preparation_groups;
    std::unordered_set<std::string> unique;
    void insert(const std::string& key, Entry entry) {
        std::string signature = key;
        signature.push_back(char(255));
        auto append = [&](int value) { for (int b = 0; b < 4; ++b) signature.push_back(char((unsigned(value) >> (8 * b)) & 255)); };
        append(entry.workers);
        for (const auto& a : entry.schedule) {
            append(a.n_units);
            for (int u = 0; u < a.n_units; ++u) { append(a.units[u].op); append(a.units[u].arg); append(a.units[u].n); }
        }
        for (const auto& e : entry.hires) {
            append(e.hour); append(e.order_index); append(e.quantity); append(e.item);
            const auto cash = std::bit_cast<uint64_t>(e.cash_delta); append(uint32_t(cash)); append(uint32_t(cash >> 32));
        }
        if (!unique.insert(std::move(signature)).second) { ++duplicates; return; }
        auto& group = entries[key];
        if (group.empty()) preparation_groups[preparation_key(key)].push_back(key);
        group.push_back(std::move(entry)); ++loaded;
        std::stable_sort(group.begin(), group.end(), [](const auto& a, const auto& b) { return a.workers < b.workers; });
    }
    void load_packed(std::istream& input) {
        namespace binary = bank_binary;
        const auto count = binary::u32(input); if (count > 200000) throw std::runtime_error("oversized packed schedule bank");
        for (uint32_t i = 0; i < count; ++i) {
            const auto key = binary::string(input, 65536); Entry entry;
            entry.workers = binary::u32(input); entry.source = binary::string(input, 4096);
            const auto hires = binary::u32(input);
            if (entry.workers < 1 || entry.workers > 40 || hires > 39) throw std::runtime_error("invalid packed workforce");
            for (uint32_t h = 0; h < hires; ++h) {
                day_solver::MarketEvent event;
                const auto hour = binary::u32(input), slot = binary::u32(input), item = binary::u32(input), quantity = binary::u32(input);
                if (hour > 23 || slot > 9 || item > kag::N_ITEMS || quantity != 1) throw std::runtime_error("invalid packed hire");
                event.hour = hour; event.order_index = slot; event.item = int(item) - 1; event.quantity = quantity; event.market_op = kag::M_HIRE;
                event.cash_delta = binary::i64(input); entry.hires.push_back(event);
            }
            for (auto& action : entry.schedule) {
                const auto units = binary::u32(input); if (units > 40) throw std::runtime_error("invalid packed worker count");
                action.n_units = units;
                for (uint32_t u = 0; u < units; ++u) {
                    const auto op = binary::u32(input), arg = binary::u32(input), quantity = binary::u32(input);
                    if (op > kag::OP_INVALID || arg > 255 || quantity > 2147483647) throw std::runtime_error("invalid packed worker command");
                    action.units[u] = {uint8_t(op), uint8_t(arg), int32_t(quantity)};
                }
                action.finalize();
            }
            insert(key, std::move(entry));
        }
        if (input.peek() != std::char_traits<char>::eof()) throw std::runtime_error("extra packed schedule-bank data");
    }
public:
    int loaded = 0, duplicates = 0, checks = 0, matches = 0, weed_repairs = 0, preparation_repairs = 0;
    void add(const DayProblem& problem, const Schedule& schedule, std::string source) {
        std::vector<day_solver::MarketEvent> hires;
        for (const auto& event : problem.market_plan) if (event.market_op == kag::M_HIRE) hires.push_back(event);
        insert(work_key(problem), {problem.worker_count, std::move(hires), schedule, std::move(source)});
    }
    void save_packed(const fs::path& path) const {
        namespace binary = bank_binary;
        std::ofstream output(path, std::ios::binary); if (!output) throw std::runtime_error("cannot write packed schedule bank");
        output.write("TPLBANK1", 8); binary::u32(output, loaded);
        std::vector<std::string> keys; for (const auto& [key, group] : entries) keys.push_back(key);
        std::sort(keys.begin(), keys.end());
        for (const auto& key : keys) for (const auto& entry : entries.at(key)) {
            binary::string(output, key); binary::u32(output, entry.workers); binary::string(output, entry.source);
            binary::u32(output, entry.hires.size());
            for (const auto& event : entry.hires) {
                binary::u32(output, event.hour); binary::u32(output, event.order_index); binary::u32(output, event.item + 1);
                binary::u32(output, event.quantity); binary::i64(output, event.cash_delta);
            }
            for (const auto& action : entry.schedule) {
                binary::u32(output, action.n_units);
                for (int u = 0; u < action.n_units; ++u) {
                    binary::u32(output, action.units[u].op); binary::u32(output, action.units[u].arg); binary::u32(output, action.units[u].n);
                }
            }
        }
        if (!output) throw std::runtime_error("failed packed schedule-bank write");
    }
    void load(const fs::path& manifest) {
        std::ifstream input(manifest, std::ios::binary); if (!input) throw std::runtime_error("cannot read schedule bank");
        std::string magic(8, '\0'); input.read(magic.data(), magic.size());
        if (magic == "TPLBANK1") { load_packed(input); return; }
        if (magic.starts_with("TPLBANK")) throw std::runtime_error("unsupported packed schedule-bank version");
        input.clear(); input.seekg(0);
        std::string path;
        while (std::getline(input, path)) {
            if (path.empty()) continue;
            for (int d = 0; d < 30; ++d) {
                const auto folder = fs::path(path) / day_name(d);
                if (!fs::exists(folder / "physical.actions.txt")) continue;
                add(day_solver::load_problem_json(folder / "problem.json"), labor::offline::read_actions((folder / "physical.actions.txt").string()), folder.string());
            }
        }
    }
    Certificate find(const Day& day, int d, int maximum_workers = 40, bool repair_preparation = false) {
        Certificate result;
        int used_repair = 0;
        const auto exact_key = work_key(day.problem), clear_key = work_key(day.problem, true);
        std::vector<std::pair<std::string, int>> groups{{exact_key, 0}};
        if (clear_key != exact_key) groups.emplace_back(clear_key, 1);
        if (repair_preparation) if (const auto found = preparation_groups.find(preparation_key(exact_key)); found != preparation_groups.end()) {
            auto keys = found->second; std::sort(keys.begin(), keys.end());
            for (const auto& key : keys) if (key != exact_key && key != clear_key) groups.emplace_back(key, 2);
        }
        int preparation_candidates = 0;
        for (const auto& [key, repair] : groups) {
          if (repair == 2 && preparation_candidates >= 64) continue;
          const auto group = entries.find(key);
          if (group == entries.end()) continue;
          for (const auto& entry : group->second) {
            if (entry.workers > maximum_workers || (result.schedule && entry.workers >= result.workers)) break;
            auto p = day.problem; p.worker_count = entry.workers;
            std::erase_if(p.market_plan, [](const auto& e) { return e.market_op == kag::M_HIRE; });
            bool collision = false;
            for (const auto& e : entry.hires) {
                const auto op = day.executable[e.hour].orders[e.order_index].op;
                if (op != kag::M_NONE && op != kag::M_HIRE) { collision = true; break; }
                p.market_plan.push_back(e);
            }
            if (collision) continue;
            if (repair == 2 && preparation_candidates++ >= 64) break;
            std::sort(p.market_plan.begin(), p.market_plan.end(), [](const auto& a, const auto& b) {
                return std::pair{a.hour, a.order_index} < std::pair{b.hour, b.order_index};
            });
            day_scheduler::prepare_problem(p);
            if (d == 29) labor::offline::require_terminal_work(p);
            auto schedule = entry.schedule; labor::offline::physical_orders(p, schedule);
            bool valid = false;
            for (int variant = 0; variant < (repair ? 3 : 1); ++variant) {
                const auto trial = repair == 2 ? repair_preparation_schedule(p, schedule, variant, d == 29 ? 22 : 23) : repair == 1 ? repair_weed_schedule(p, schedule, variant) : std::optional<Schedule>(schedule);
                if (!trial) continue;
                const auto replay = day_solver::replay_schedule(p, *trial); ++checks;
                if (replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty()) {
                    schedule = *trial; valid = true; break;
                }
            }
            if (!valid) continue;
            result.workers = entry.workers; result.problem = std::move(p); result.schedule = schedule; result.initial_witness = entry.source;
            used_repair = repair; break;
          }
        }
        if (result.schedule) { ++matches; weed_repairs += used_repair == 1; preparation_repairs += used_repair == 2; }
        return result;
    }
};
}
