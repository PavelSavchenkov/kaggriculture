#include "policy.hpp"
#include <iostream>
int main(){
    int cases;if(!(std::cin>>cases))return 2;
    for(int c=0;c<cases;++c){
        int step;kag::Action action;std::cin>>step>>action.n_orders;action.n_units=1;
        for(int i=0;i<action.n_orders;++i){int op,item;std::cin>>op>>item>>action.orders[i].n;action.orders[i].op=op;action.orders[i].item=item;}
        action.finalize();if(step>=144)compositions::premium_sales::prioritize(action);
        std::cout<<action.n_orders;
        for(int i=0;i<action.n_orders;++i){const auto& m=action.orders[i];std::cout<<' '<<int(m.op)<<' '<<int(m.item)<<' '<<m.n;}
        std::cout<<'\n';
    }
    return std::cin?0:3;
}
