#include "schedule_bank.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 4 || fs::exists(argv[2])) throw std::runtime_error("usage: pack_bank input_manifest new_packed_bank validation_manifest");
        const auto began = std::chrono::steady_clock::now();
        ScheduleBank original; original.load(argv[1]);
        const double original_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        original.save_packed(argv[2]);
        const auto packed_began = std::chrono::steady_clock::now();
        ScheduleBank packed; packed.load(argv[2]);
        const double packed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - packed_began).count();
        if (original.loaded != packed.loaded) throw std::runtime_error("packed bank changed the entry count");
        auto same_schedule = [](const Schedule& a, const Schedule& b) {
            for (int h = 0; h < 24; ++h) {
                if (a[h].n_units != b[h].n_units || a[h].n_orders != b[h].n_orders) return false;
                for (int u = 0; u < a[h].n_units; ++u) {
                    const auto& x = a[h].units[u]; const auto& y = b[h].units[u];
                    if (x.op != y.op || x.arg != y.arg || x.n != y.n) return false;
                }
                for (int s = 0; s < a[h].n_orders; ++s) {
                    const auto& x = a[h].orders[s]; const auto& y = b[h].orders[s];
                    if (x.op != y.op || x.item != y.item || x.n != y.n) return false;
                }
            }
            return true;
        };
        std::ifstream manifest(argv[3]); if (!manifest) throw std::runtime_error("cannot read validation manifest");
        std::string path; int checked = 0, matches = 0;
        while (std::getline(manifest, path)) if (!path.empty()) for (int d = 0; d < 30; ++d) {
            const auto folder = fs::path(path) / day_name(d);
            Day day; day.problem = day_solver::load_problem_json(folder / "problem.json");
            day.executable = labor::offline::read_actions((folder / "executable.actions.txt").string());
            day.menu = legal_menu(day, d == 29 ? 23 : 24);
            const auto a = original.find(day, d), b = packed.find(day, d); ++checked;
            if (bool(a.schedule) != bool(b.schedule) || a.workers != b.workers || a.initial_witness != b.initial_witness ||
                (a.schedule && !same_schedule(*a.schedule, *b.schedule))) throw std::runtime_error("packed bank changed a lookup result");
            matches += bool(a.schedule);
        }
        std::cout << "{\"entries\":" << original.loaded << ",\"bytes\":" << fs::file_size(argv[2]) << ",\"original_load_seconds\":" << original_seconds
                  << ",\"packed_load_seconds\":" << packed_seconds << ",\"checked\":" << checked << ",\"matches\":" << matches << "}\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
