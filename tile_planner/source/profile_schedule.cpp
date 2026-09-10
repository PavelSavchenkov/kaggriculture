#include "weed_schedule_repair.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3 || fs::exists(argv[2])) throw std::runtime_error("usage: profile_schedule day_folder new_output");
        const fs::path source(argv[1]), output(argv[2]); fs::create_directories(output);
        const auto problem = day_solver::load_problem_json(source / "problem.json");
        const auto schedule = labor::offline::read_actions((source / "physical.actions.txt").string());
        const auto replay = day_solver::replay_schedule(problem, schedule);
        if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty()) throw std::runtime_error("invalid source schedule");
        const auto positions = schedule_positions(schedule);
        struct Worker { int birth = -1, field = 0, moves = 0, resources = 0, idle = 0; std::array<int, 4> quadrant{}; std::array<bool, 100> tiles{}; };
        std::array<Worker, 40> workers;
        std::ofstream steps(output / "steps.csv"), table(output / "workers.csv"), tiles(output / "tiles.csv");
        steps << "hour,worker,x,y,op,arg,quantity,field\n";
        for (int h = 0; h < 24; ++h) for (int u = 0; u < schedule[h].n_units; ++u) {
            auto& worker = workers[u]; const auto& action = schedule[h].units[u]; const int cell = positions[h][u];
            if (worker.birth < 0) worker.birth = h;
            const bool field = (action.op >= kag::OP_PLANT && action.op <= kag::OP_CARE) || (action.op == kag::OP_PLACE && kag::is_animal(action.arg));
            if (field) { ++worker.field; ++worker.quadrant[quadrant(cell)]; worker.tiles[cell] = true; }
            else if (action.op >= kag::OP_NORTH && action.op <= kag::OP_WEST) ++worker.moves;
            else if (action.op == kag::OP_PASS) ++worker.idle;
            else ++worker.resources;
            steps << h << ',' << u << ',' << cell % 10 << ',' << cell / 10 << ',' << +action.op << ',' << +action.arg << ',' << action.n << ',' << field << '\n';
        }
        table << "worker,birth_hour,field,moves,resources,idle,tiles,q0,q1,q2,q3\n";
        for (int u = 0; u < 40; ++u) if (workers[u].birth >= 0) {
            const auto& w = workers[u];
            table << u << ',' << w.birth << ',' << w.field << ',' << w.moves << ',' << w.resources << ',' << w.idle << ',' << std::count(w.tiles.begin(), w.tiles.end(), true);
            for (int count : w.quadrant) table << ',' << count;
            table << '\n';
        }
        tiles << "x,y,kind,crop,animal,field_actions,radius\n";
        for (const auto& work : problem.tile_work) {
            const auto& tile = problem.start.managed_tiles[work.tile];
            tiles << +tile.x << ',' << +tile.y << ',' << int(tile.state.kind) << ',' << tile.state.crop << ',' << tile.state.animal << ',' << work.actions.size()
                  << ',' << labor::shed_distance(tile.y * 10 + tile.x) << '\n';
        }
        std::cout << "profiled " << problem.worker_count << " certified workers\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
