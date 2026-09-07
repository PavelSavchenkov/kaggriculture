#include "context.hpp"
#include "problem_json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <boost/json.hpp>

namespace json = boost::json;

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: semantic_context_cli v3_problem.json output.json");
        // Destroy the parsed input before serializing the returned context.
        const auto context = [] (const char* input) {
            const auto problem = day_solver::load_problem_json(input);
            return day_native::semantic::prepare(problem);
        }(argv[1]);
        json::array tasks;
        json::object groups, deadlines, late;
        for (int id = 0; id < int(context.tasks.size()); ++id) {
            const auto& task = context.tasks[id];
            tasks.push_back({{"id", id}, {"tile", task.tile}, {"pattern", task.pattern}, {"x", task.x}, {"y", task.y},
                {"input_item", task.input}, {"output_item", task.output}, {"predecessor", task.predecessor}, {"action_class", int(task.action)}});
        }
        for (const auto& [item, group] : context.groups) groups[std::to_string(item)] = {
            {"sources", json::value_from(group.sources)}, {"targets", json::value_from(group.targets)}};
        for (auto [task, hour] : context.deadlines) deadlines[std::to_string(task)] = hour;
        for (auto [task, hour] : context.late_inputs) late[std::to_string(task)] = hour;
        if (std::filesystem::exists(argv[2])) throw std::runtime_error("output already exists");
        std::ofstream output(argv[2]);
        output << json::serialize(json::object{{"tasks", tasks}, {"groups", groups}, {"deadlines", deadlines}, {"late_inputs", late}}) << '\n';
        if (!output) throw std::runtime_error("cannot write output");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
