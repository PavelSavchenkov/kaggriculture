#include "source/policy.hpp"
#include "../../candidates/junghoon_wool_sales/source/agent.hpp"
#include "../../include/evaluation.hpp"
#include <fstream>

void orders(std::ostream& out,const kag::Action& a){
    out<<'[';for(int i=0;i<a.n_orders;++i){if(i)out<<',';const auto& q=a.orders[i];out<<'['<<int(q.op)<<','<<int(q.item)<<','<<q.n<<']';}out<<']';
}
void trace(std::ostream& out,int mode){
    kag::Config config;config.seed=1044;kag::Sim sim(config);
    compositions::observed_sale_lead_shared::Policy agent(mode);
    compositions::junghoon_wool_sales::Agent rival;
    agent.reset(kag::agent::runtime::make_agent_init(sim,0));rival.reset(kag::agent::runtime::make_agent_init(sim,1));
    kag::agent::DecisionBudget budget;budget.max_expansions=100000;
    uint64_t rng=1044^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops{};
    for(auto& shop:shops)shop=compositions::random_word(rng)%kag::N_SHOPS;
    out<<'[';bool first=true;
    while(!sim.st.done){
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        auto oa=kag::agent::runtime::make_observation(sim,0),ob=kag::agent::runtime::make_observation(sim,1);
        kag::Action a,b;agent.act(oa,budget,a);rival.act(ob,budget,b);
        if(!first)out<<",\n";first=false;
        out<<"{\"step\":"<<sim.st.step<<",\"cash_before\":["<<sim.st.farms[0].money<<','<<sim.st.farms[1].money<<"],\"orders\":[";
        orders(out,a);out<<',';orders(out,b);out<<"],\"wool_price\":"<<oa.market.prices[kag::WOOL];
        auto before=sim;sim.step(a,b);
        out<<",\"cash_after\":["<<sim.st.farms[0].money<<','<<sim.st.farms[1].money<<"],\"rival_revenue\":"<<sim.st.farms[1].sell_revenue-before.st.farms[1].sell_revenue
           <<",\"rival_spend\":"<<sim.st.farms[1].total_spend-before.st.farms[1].total_spend<<",\"rival_shed_sheep_before\":"<<before.st.farms[1].shed[kag::SHEEP]
           <<",\"rival_shed_sheep_after\":"<<sim.st.farms[1].shed[kag::SHEEP]<<'}';
    }
    out<<']';
}
int main(int argc,char** argv){if(argc!=2)return 2;std::ofstream out(argv[1]);out<<"{\"control\":";trace(out,0);out<<",\"lead\":";trace(out,1);out<<"}\n";}
