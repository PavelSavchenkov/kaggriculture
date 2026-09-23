#include "forecast.hpp"
#include "forecast_weights.hpp"
#include <algorithm>
#include <cmath>

namespace kag::day_compiler {
namespace {
double pending_stock(const Observation& o,const History& history,int product,const Configuration& config) {
    double stock=0;
    for(int step=std::max(0,o.step-24);step<o.step;++step) {
        const auto& sample=history.samples()[step%24]; if(sample.step!=step) continue;
        const double sold=std::max(0.0,(sample.lower[product]+sample.upper[product])*.5);
        stock=std::max(0.0,stock+sample.rival_visible_removal[product]-sold);
        if(step%24==23) stock=std::min(double(config.shed_capacity),stock);
    }
    return stock;
}
}
FlowFeatures flow_features(const Observation& o,const History& history,int product,int horizon,const Configuration& config) {
    FlowFeatures x{};
    x[0]=o.day; x[1]=o.hour; x[2]=horizon; x[3]=o.market.inventory[product]-10000;
    x[4]=o.market.prices[product]; x[5]=demand(o,config,product,4);
    x[10]=o.opponent().n_units; x[13]=o.opponent().money/1000;
    x[21]=o.own.shed[product]; x[22]=config.episode_steps-2-o.step;
    for(int u=0;u<o.opponent().n_units;++u) {
        const int d=shed_distance(o.opponent().pos_y[u]*BOARD+o.opponent().pos_x[u]);
        x[11]+=d==0; x[12]+=d<=2;
    }
    for(int cell=0;cell<BOARD*BOARD;++cell) {
        const auto& own=o.self().tiles[cell/BOARD][cell%BOARD];
        const int own_product=own.kind==T_PLANT?own.what:own.has_animal?ANIMALS[own.what-GOOSE].product:-1;
        if(own_product==product) x[20]+=own.yield_units;
        const auto& tile=o.opponent().tiles[cell/BOARD][cell%BOARD];
        const int p=tile.kind==T_PLANT?tile.what:tile.has_animal?ANIMALS[tile.what-GOOSE].product:-1;
        if(p!=product) continue;
        x[6]+=tile.yield_units; ++x[7]; x[8]+=tile.yield_units?shed_distance(cell):0;
        const int age=o.day-tile.planted_day;
        if(tile.has_animal) {
            const auto& species=ANIMALS[tile.what-GOOSE];
            int next=std::max(0,species.first_yield_day-age);
            while(!next || (age+next-species.first_yield_day)%species.interval) ++next;
            if(next<=horizon/24+1) x[9]+=1+tile.pending_care_bonus;
            x[23]+=tile.pending_care_bonus;
        } else if(age<CROPS[product].first_yield_day && CROPS[product].first_yield_day-age<=horizon/24+1)
            x[9]+=tile.yield_units;
    }
    for(const auto& sample:history.samples()) {
        const int age=o.step-1-sample.step;
        if(sample.step<0 || age<0 || age>=24) continue;
        const double flow=std::max(0.0,(sample.lower[product]+sample.upper[product])*.5);
        if(age<4) x[14]+=sample.rival_visible_removal[product];
        if(age<12) x[15]+=sample.rival_visible_removal[product];
        if(age==0) x[16]+=flow;
        if(age<4) x[17]+=flow;
        if(age<12) x[18]+=flow;
        x[19]+=flow;
    }
    return x;
}
double predict_flow(int product,const FlowFeatures& x) {
    if(product<CARROT || product>WOOL) return 0;
    double result=forecast_weights::base[product];
    for(int tree=0;tree<forecast_weights::counts[product];++tree) {
        int node=forecast_weights::roots[product][tree];
        while(!forecast_weights::nodes[node].leaf) {
            const auto& branch=forecast_weights::nodes[node];
            node=x[branch.feature]<=branch.threshold?branch.left:branch.right;
        }
        result+=forecast_weights::nodes[node].value;
    }
    return std::max(0.0,result);
}
void forecast_sales(const Observation& o,const History& history,int product,int hours,int* sales,const Configuration& config,bool terminal_balance) {
    auto features=flow_features(o,history,product,1,config);
    int cumulative=0;
    for(int h=0;h<hours;++h) {
        features[2]=h+1;
        if(h==23) features=flow_features(o,history,product,24,config);
        const int predicted=std::max(cumulative,int(std::lround(predict_flow(product,features))));
        sales[h]=std::clamp(predicted-cumulative,0,100); cumulative+=sales[h];
    }
    if(!terminal_balance || o.day!=29) return;
    const double stock=pending_stock(o,history,product,config);
    int field=0;
    for(int cell=0;cell<100;++cell) {
        const auto& tile=o.opponent().tiles[cell/10][cell%10];
        const int p=tile.kind==T_PLANT?tile.what:tile.has_animal?ANIMALS[tile.what-GOOSE].product:-1;
        if(p!=product || !tile.yield_units) continue;
        if(tile.kind==T_PLANT && o.day-tile.planted_day<CROPS[p].first_yield_day) continue;
        int arrival=100;
        for(int u=0;u<o.opponent().n_units;++u)
            arrival=std::min(arrival,distance(cell,o.opponent().pos_y[u]*BOARD+o.opponent().pos_x[u]));
        if(o.opponent().money>=1) arrival=std::min(arrival,1+shed_distance(cell));
        if(arrival+1+shed_distance(cell)>=hours) continue;
        field+=tile.yield_units;
    }
    // Terminal cargo cannot wait for night settlement. Add the unrepresented
    // public supply as a final return wave, following the old suffix planner's
    // causal supply-balance idea. This is an explicit hypothesis, not a receipt.
    int extra=std::max(0,int(std::lround(stock))+field-cumulative);
    for(int h=std::max(0,18-o.hour);h<hours && extra;++h) {
        const int quantity=std::min(100-sales[h],(extra+hours-h-1)/(hours-h));
        sales[h]+=quantity; extra-=quantity;
    }
}
void cover_visible_supply(const Observation& o,const History& history,int product,int hours,int* sales,const Configuration& config) {
    int receipts[24]{};
    receipts[0]=std::min(config.shed_capacity,int(std::lround(pending_stock(o,history,product,config))));
    for(int cell=0;cell<100;++cell) {
        const auto& tile=o.opponent().tiles[cell/10][cell%10];
        const bool fertilizer=product==FERTILIZER;
        const int p=fertilizer?(tile.has_animal?FERTILIZER:-1):
            tile.kind==T_PLANT?tile.what:tile.has_animal?ANIMALS[tile.what-GOOSE].product:-1;
        const int quantity=fertilizer?int(tile.fertilizer_available):int(tile.yield_units);
        if(p!=product || !quantity) continue;
        const int age=o.day-tile.planted_day;
        if(tile.kind==T_PLANT && age<CROPS[p].first_yield_day) continue;
        int arrival=100;
        for(int u=0;u<o.opponent().n_units;++u)
            arrival=std::min(arrival,distance(cell,o.opponent().pos_y[u]*BOARD+o.opponent().pos_x[u]));
        if(o.opponent().money>=1) arrival=std::min(arrival,1+shed_distance(cell));
        const int due=arrival+1+shed_distance(cell);
        if(due>=hours) continue;
        receipts[due]+=quantity;
    }
    int model=0,visible=0,delivered=0;
    for(int h=0;h<hours;++h) {
        model+=sales[h]; visible+=receipts[h];
        sales[h]=std::clamp(std::max(model,visible)-delivered,0,config.shed_capacity);
        delivered+=sales[h];
    }
}
}
