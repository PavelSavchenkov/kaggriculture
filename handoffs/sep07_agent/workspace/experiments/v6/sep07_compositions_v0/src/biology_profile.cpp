#include "../include/biology.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

struct Proposal {
    uint64_t episode;
    int seat,rank;
    double recorded_cash;
    std::vector<compositions::Cohort> cohorts;
};

int main(int argc, char** argv) {
    if (argc!=3) return 2;
    std::ifstream input(argv[1]);
    int n; input >> n;
    if (!input || n<1 || n>10000) return 2;
    std::vector<Proposal> proposals(n);
    for (auto& p : proposals) {
        int count; input >> p.episode >> p.seat >> p.rank >> p.recorded_cash >> count;
        if (!input || count<1 || count>10000) return 2;
        p.cohorts.resize(count);
        for (auto& c : p.cohorts) {
            int item; input >> item >> c.count >> c.start_day >> c.end_day;
            c.item=item;
        }
    }
    if (!input) return 2;
    std::ofstream output(argv[2]);
    output << "episode,seat,rank,recorded_cash,fertilize,wheat_input,fertilizer_input,field_operations,purchase_cost,peak_occupied";
    for (int i=0; i<kag::N_PRODUCTS; ++i) output << ",output_" << i;
    output << '\n';
    int checksum=0;
    const auto start=std::chrono::steady_clock::now();
    constexpr int repeats=100;
    for (int repeat=0; repeat<repeats; ++repeat) for (const auto& p : proposals) for (int fert=0; fert<2; ++fert) {
        int products[kag::N_PRODUCTS]{}, wheat=0, fertilizer=0, work=0,cost=0;
        int occupancy[30]{};
        for (const auto& cohort : p.cohorts) {
            auto result=compositions::biology(cohort,compositions::productive_service(cohort,fert));
            cost += result.seed_cost+result.animal_cost;
            for (int day=0; day<30; ++day) {
                const auto& d=result.days[day];
                for (int i=0; i<kag::N_PRODUCTS; ++i) products[i]+=d.output[i];
                wheat+=d.wheat; fertilizer+=d.fertilizer; work+=d.operations; occupancy[day]+=d.active;
            }
        }
        checksum+=products[0];
        if (repeat) continue;
        output << p.episode << ',' << p.seat << ',' << p.rank << ',' << p.recorded_cash << ',' << fert
               << ',' << wheat << ',' << fertilizer << ',' << work << ',' << cost << ',' << *std::max_element(occupancy,occupancy+30);
        for (int i=0; i<kag::N_PRODUCTS; ++i) output << ',' << products[i];
        output << '\n';
    }
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout << "profiles=" << proposals.size()*2 << " repetitions=" << repeats
              << " microseconds_per_profile=" << seconds*1e6/(repeats*proposals.size()*2)
              << " checksum=" << checksum << '\n';
}
