#include "policy.hpp"
#include "../../../include/evaluation.hpp"
#include "../../../league/public_router/source/agent.hpp"
#include <iomanip>

using namespace compositions;
using namespace compositions::late_portfolio;

template<class Opponent> int run(const Options& o,int forced) {
    const int seats=o.seat_mode==2?2:1,count=int(o.seeds.size())*seats;
    std::vector<Outcome> outcomes(count);std::vector<Prediction> predictions(count);
    std::vector<int> choices(count);std::vector<uint32_t> matches(count);
    std::atomic<int> next{0};
    const auto begin=std::chrono::steady_clock::now();
    auto worker=[&] {
        Policy policy(32,0,0,0,forced);Opponent opponent;
        for(;;) {
            const int i=next.fetch_add(1);if(i>=count)break;
            outcomes[i]=run_game(policy,opponent,o.seeds[i/seats],seats==2?i%2:o.seat_mode,o);
            predictions[i]=policy.prediction();choices[i]=policy.choice();matches[i]=policy.matched_days();
        }
    };
    std::vector<std::thread> threads;
    for(int i=0;i<std::min(count,o.threads);++i)threads.emplace_back(worker);
    for(auto& thread:threads)thread.join();
    write_results(o,outcomes,std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count());
    std::ofstream csv(o.output+".predictions.csv");if(!csv)std::abort();
    csv<<std::setprecision(12)<<"seed,seat,choice,matched_days,estimate_seconds";
    for(int c=0;c<4;++c)for(const char* metric:{"eligible","own","margin","deviation","positive_fraction"})csv<<','<<metric<<'_'<<c;
    csv<<'\n';
    for(int i=0;i<count;++i) {
        const auto& p=predictions[i];csv<<outcomes[i].seed<<','<<outcomes[i].seat<<','<<choices[i]<<','<<matches[i]<<','<<p.seconds;
        for(int c=0;c<4;++c)csv<<','<<p.eligible[c]<<','<<p.own[c]<<','<<p.margin[c]<<','<<p.deviation[c]<<','<<p.positive_fraction[c];
        csv<<'\n';
    }
    return 0;
}
int main(int argc,char** argv) {
    const auto o=options(argc,argv);const int forced=std::stoi(o.a);
    if(forced<-1 || forced>3)return 2;
    if(o.b=="opening_q32_b13_v1")return run<opening_q32_b13_v1::Agent>(o,forced);
    if(o.b=="public_router")return run<public_router::Agent>(o,forced);
    return 2;
}
