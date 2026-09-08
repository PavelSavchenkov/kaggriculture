#include "policy/source/agent.hpp"
#include "evaluation.hpp"
#include <filesystem>
#include <sstream>

struct Choice { std::string name; int quantity, buffer, mode; };

int main(int argc, char** argv) {
    if (argc != 5 || std::filesystem::exists(argv[1])) return 2;
    const std::filesystem::path out(argv[1]); std::filesystem::create_directories(out);
    std::ifstream input(argv[2]); if (!input) return 2;
    std::string line; std::getline(input,line);
    if (line != "name,quantity,buffer,mode") return 2;
    std::vector<Choice> choices;
    while (std::getline(input,line)) {
        std::replace(line.begin(),line.end(),',',' ');
        std::istringstream row(line); Choice choice;
        if (!(row >> choice.name >> choice.quantity >> choice.buffer >> choice.mode)) return 2;
        choices.push_back(choice);
    }
    if (choices.size() != 19) return 2;
    compositions::Options options; options.games=std::stoi(argv[3]); options.native_shops=std::stoi(argv[4]); options.threads=8; options.validate=true;
    const uint64_t first=options.native_shops?1773000:1770000;
    for (uint64_t seed=first;seed<first+options.games;++seed) options.seeds.push_back(seed);
    for (size_t i=9;i==9;++i) for (size_t j=0;j<choices.size();++j) {
        const auto a=choices[i],b=choices[j]; options.a=a.name; options.b=b.name;
        options.output=(out/(a.name+"_vs_"+b.name+".json")).string();
        compositions::run_batch(options,
            [=] { return compositions::opening_market_search::Agent(a.quantity,a.buffer,a.mode); },
            [=] { return compositions::opening_market_search::Agent(b.quantity,b.buffer,b.mode); });
    }
}
