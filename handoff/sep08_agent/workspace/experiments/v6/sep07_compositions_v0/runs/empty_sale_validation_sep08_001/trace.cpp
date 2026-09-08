#include "../../include/evaluation.hpp"
#include "../empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs = std::filesystem;
using Parent = kag::agents::observed_sale_lead_start_216::Agent;

void orders(std::ostream& out, const kag::Action& action) {
    out << '[';
    for (int i=0; i<action.n_orders; ++i) {
        if(i) out << ',';
        const auto& o=action.orders[i];
        out << '[' << +o.op << ',' << +o.item << ',' << o.n << ']';
    }
    out << ']';
}

template<class Own> void trace(const fs::path& dir, int mode) {
    constexpr int seed=2004097, seat=0;
    std::string name="m"+std::to_string(mode)+"_2004097_s0";
    std::ofstream out(dir/(name+".jsonl"));
    kag::Config config; config.seed=seed;
    kag::Sim sim(config);
    Own own; Parent rival;
    own.reset(kag::agent::runtime::make_agent_init(sim,seat));
    rival.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    while(!sim.st.done) {
        auto a=kag::agent::runtime::make_observation(sim,seat);
        auto b=kag::agent::runtime::make_observation(sim,seat^1);
        // Diagnostic copies receive observations only. Sim copies stay here.
        Parent probe=static_cast<const Parent&>(own);
        kag::Action raw, actions[2];
        probe.act(a,{},raw);
        own.act(a,{},actions[seat]); rival.act(b,{},actions[seat^1]);
        validate_action(actions[seat],a); validate_action(actions[seat^1],b);
        out << "{\"step\":" << sim.st.step << ",\"cash_before\":[" << a.self().money << ',' << b.self().money
            << "],\"inventory\":[";
        for(int i=0;i<kag::N_PRODUCTS;++i){if(i)out<<',';out<<a.market.inventory[i];}
        out << "],\"raw_orders\":"; orders(out,raw);
        out << ",\"own_orders\":"; orders(out,actions[seat]);
        out << ",\"rival_orders\":"; orders(out,actions[seat^1]);
        for(int p=0;p<2;++p)hash_action(hashes[p],actions[p]);
        auto counterfactual=sim;
        counterfactual.step(raw,actions[seat^1]);
        sim.step(actions[0],actions[1]);
        out << ",\"cash_after\":[" << sim.st.farms[0].money << ',' << sim.st.farms[1].money
            << "],\"cash_raw_order_counterfactual\":[" << counterfactual.st.farms[0].money << ','
            << counterfactual.st.farms[1].money << "]}\n";
    }
    std::ofstream end(dir/(name+".json"));
    end << "{\"seed\":" << seed << ",\"seat\":" << seat << ",\"mode\":" << mode
        << ",\"cash\":" << sim.st.farms[seat].money << ",\"opponent_cash\":" << sim.st.farms[seat^1].money
        << ",\"action_hash\":\"" << hashes[seat] << "\",\"opponent_action_hash\":\"" << hashes[seat^1] << "\"}\n";
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;
    fs::path dir=argv[1]; if(fs::exists(dir))return 2; fs::create_directories(dir);
    trace<Parent>(dir,0); trace<compositions::empty_sale_slots_m2::Agent>(dir,2);
}
