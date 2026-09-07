#define VERIFY_KING
#include "../source/agent.hpp"
#include "../source/audit.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){if(argc!=5)return 2;std::ifstream in(argv[1]);const int branch=std::stoi(argv[2]),mode=std::stoi(argv[3]);std::ofstream state(argv[4]);compositions::atakan_portfolio::Agent agent(mode);
    int cases = 0, reset;
    while (in >> reset) {
        kag::agent::AgentObservation o{};
        int player;
        in >> player >> o.step >> o.day >> o.hour;
        o.player = player;
        for (auto& f : o.farms) {
            in >> f.money >> f.n_units >> f.n_quadrants >> f.hires_today;
            for (int u = 0; u < f.n_units; ++u) { int x,y; in >> x >> y; f.pos_x[u]=x; f.pos_y[u]=y; }
            for (auto& row : f.tiles) for (auto& t : row) {
                int kind; in >> kind; t.kind=kag::TileKind(kind);
#if defined(VERIFY_KAITO_V58) || defined(VERIFY_CAPACITY) || defined(VERIFY_FINANCE7) || defined(VERIFY_KING) || defined(VERIFY_BOATLEE_V29)
                int what,animal,water,fed,care,fert,dry,yield,bonus,planted,maxlife,until;
                in>>what>>animal>>water>>fed>>care>>fert>>dry>>yield>>bonus>>planted>>maxlife>>until;
                t.what=what;t.has_animal=animal;t.watered_today=water;t.fed_today=fed;t.cared_today=care;
                t.fertilizer_available=fert;t.consecutive_dry=dry;t.yield_units=yield;t.pending_care_bonus=bonus;
                t.planted_day=planted;t.max_lifespan_step=maxlife;t.fertilized_until_day=until;
#endif
            }
        }
        for (auto& v : o.own.shed) { int n; in >> n; v=n; o.own.shed_total+=n; }
        for (auto& v : o.own.seeds) { int n; in >> n; v=n; }
        for (int u=0; u<o.self().n_units; ++u) {
            int count; in >> count; o.own.inv_nkeys[u]=count;
            for (int k=0; k<count; ++k) { int item,n; in >> item >> n; o.own.inv_keys[u][k]=item; o.own.inv[u][item]=n; }
        }
        for (auto& v : o.market.prices) in >> v;
        for (auto& v : o.market.inventory) in >> v;
        in >> o.n_shops;
        for (int s=0; s<o.n_shops; ++s) { int shop; in >> shop; o.shops[s]=shop; }
#ifdef VERIFY_LIBRARY
        if(reset)agent=compositions::top_replay_library::Agent(reset-1);
#endif
        if (reset) agent.reset({{},o.player});
        if(o.step==226){auto v=compositions::atakan_portfolio::physical(o);state<<"[";for(size_t i=0;i<v.size();++i){if(i)state<<",";state<<v[i];}state<<"]\n";
            if(mode==3){o.n_shops=2;o.shops[0]=branch==1?kag::SHOP_YARN_STORE:branch==0?kag::SHOP_ICE_CREAM_SHOP:kag::SHOP_BAKERY;o.shops[1]=branch==0?kag::SHOP_SMOOTHIE_SHOP:kag::SHOP_PET_CAFE;}
            if(mode==4){o.market.prices[kag::WOOL]=branch==1?200:190;o.market.prices[kag::MILK]=branch==0?200:100;}
        }
        kag::Action a;
        agent.act(o,{},a);
        int units,orders; in >> units >> orders;
        bool match = units==a.n_units && orders==a.n_orders;
        for (int u=0; u<units; ++u) {
            int op,arg,n; in >> op >> arg >> n;
            const auto actual=a.units[u];
            bool same = actual.op==op && actual.arg==arg && actual.n==n;
#if defined(VERIFY_TEAMMATE) || defined(VERIFY_SIXDAY) || defined(VERIFY_KAITO_V58) || defined(VERIFY_FINANCE7) || defined(VERIFY_KING) || defined(VERIFY_BOATLEE_V29)
            same = actual.op==op;
            if (op==kag::OP_PICKUP || op==kag::OP_PLACE) same &= actual.arg==arg && actual.n==n;
            if (op==kag::OP_PLANT) same &= actual.arg==arg;
#endif
            match &= same;
            if (!same) std::cerr << "unit " << u << " expected " << op << ',' << arg << ',' << n << " actual " << +actual.op << ',' << +actual.arg << ',' << actual.n << '\n';
        }
        for (int i=0; i<orders; ++i) {
            int op,item,n; in >> op >> item >> n;
            const auto actual=a.orders[i];
            bool same = actual.op==op && actual.item==item && actual.n==n;
#if defined(VERIFY_TEAMMATE) || defined(VERIFY_SIXDAY) || defined(VERIFY_KAITO_V58) || defined(VERIFY_FINANCE7) || defined(VERIFY_KING) || defined(VERIFY_BOATLEE_V29)
            if (op==kag::M_NONE || op==kag::M_HIRE || op==kag::M_BUY_LAND) same = actual.op==op;
#endif
            match &= same;
            if (!same) std::cerr << "order " << i << " expected " << op << ',' << item << ',' << n << " actual " << +actual.op << ',' << +actual.item << ',' << actual.n << '\n';
        }
        if (!in || !match) { std::cerr << "mismatch case=" << cases << " step=" << o.step << "\n"; return 1; }
        ++cases;
    }
    std::cout << "matched_actions=" << cases << "\n";
    return cases ? 0 : 1;
}
