#include <chrono>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>

#include <boost/json.hpp>

#include "problem_json.hpp"
#include "problem_validation.hpp"
#include "tile_graph.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: tile_graph_cli PROBLEM.json|-\n";
        return 2;
    }
    try {
        const auto started = std::chrono::steady_clock::now();
        const auto problem = std::string(argv[1]) == "-"
            ? day_solver::parse_problem_json(std::string(
                std::istreambuf_iterator<char>(std::cin),
                std::istreambuf_iterator<char>()))
            : day_solver::load_problem_json(argv[1]);
        const auto issues = day_solver::validate_problem(problem);
        if (problem.format_version != 3 || !issues.empty()) {
            std::cerr << "A valid v3 input is required\n";
            return 2;
        }
        const auto graphs = day_solver::build_tile_graphs(problem);
        boost::json::array tiles;
        bool feasible = true;
        for (const auto& graph : graphs) {
            boost::json::array nodes, arcs, terminals;
            for (const auto& node : graph.nodes)
                nodes.push_back({node.hour, node.prefix});
            for (const auto& arc : graph.arcs)
                arcs.push_back({arc.from, arc.prefix, arc.to});
            for (int node : graph.terminals) terminals.push_back(node);
            feasible &= !graph.terminals.empty();
            tiles.push_back({{"tile", graph.tile}, {"task_count", graph.task_count},
                             {"nodes", nodes}, {"transitions", arcs},
                             {"start", 0}, {"terminals", terminals}});
        }
        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << boost::json::serialize(boost::json::object{
            {"status", feasible ? "LOCAL_FEASIBLE" : "LOCAL_INFEASIBLE"},
            {"global_proof", false}, {"seconds", seconds}, {"tiles", tiles},
            {"scope", "Tile transitions with travel, cargo and seeds relaxed; random night weed arrivals unresolved"}
        }) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
