// Literal budget logic from Arlene V4; see ../LINEAGE.json.
void budget_guard(const kag::agent::AgentObservation& obs,const Tables& data,int route,kag::Action& action) {
    using namespace kag;
    if(obs.step%72)return;
    const int end=std::min(720,obs.step+72);
    int balance[N_ITEMS]{},need[N_ITEMS]{},hires[3]{},quadrants=obs.self().n_quadrants;
    double budget=0;
    constexpr int seeds[]={10,20,50,100,80},animals[]={300,400,500},land[]={1000,2000,4000};
    for(int turn=obs.step;turn<end;++turn) {
        const auto& planned=data.actions[route][turn];
        for(int u=0;u<planned.n_units;++u) {
            auto a=planned.units[u];int item=-1;
            if(a.op==OP_FEED)item=WHEAT;
            else if(a.op==OP_FERTILIZE)item=FERTILIZER;
            else if(a.op==OP_PLACE && is_animal(a.arg))item=a.arg;
            if(item>=0){--balance[item];need[item]=std::max(need[item],-balance[item]);}
        }
        for(int m=0;m<planned.n_orders;++m) {
            auto order=planned.orders[m];int quantity=std::max(1,order.n);
            if(order.op==M_HIRE)++hires[(turn-obs.step)/24];
            else if(order.op==M_BUY_LAND) {
                int extra=quadrants-1;if(extra>=0 && extra<3){budget+=land[extra];++quadrants;}
            } else if(order.op==M_BUY_SEED && order.item<5)budget+=seeds[order.item]*quantity;
            else if(order.op==M_BUY_PRODUCT && (order.item==WHEAT || order.item==FERTILIZER)) {
                budget+=std::max(0,obs.market.prices[order.item])*quantity;balance[order.item]+=quantity;
            } else if(order.op==M_BUY_ANIMAL && is_animal(order.item)) {
                budget+=animals[order.item-GOOSE]*quantity;balance[order.item]+=quantity;
            }
        }
    }
    for(int day=0;day<3;++day) {
        int offset=day==0?obs.self().hires_today:0;
        double a=1,b=1;
        for(int index=0;index<offset+hires[day];++index) {
            if(index>=offset)budget+=a;
            double next=a+b;a=b;b=next;
        }
    }
    double cash=obs.self().money;
    for(int item=0;item<N_PRODUCTS;++item) {
        int planned=std::max(0,data.future[route][obs.step][item]-data.future[route][end][item]);
        cash+=std::min(std::max(0,int(obs.own.shed[item])),planned)*std::max(0,obs.market.prices[item]);
    }
    double shortfall=budget-cash;if(shortfall<=0)return;
    int existing[N_PRODUCTS]{},carried[N_PRODUCTS]{},available[N_PRODUCTS]{};
    for(int m=0;m<action.n_orders;++m) {
        auto order=action.orders[m];
        if(order.op==M_SELL && order.item<N_PRODUCTS)existing[order.item]+=std::max(0,order.n);
    }
    for(int u=0;u<obs.self().n_units;++u)for(int item=0;item<N_PRODUCTS;++item)
        carried[item]+=std::max(0,int(obs.own.inv[u][item]));
    std::array<int,N_PRODUCTS> priority{};int count=0;
    for(int item=0;item<N_PRODUCTS;++item) {
        int protected_stock=std::max(0,need[item]-carried[item]);
        available[item]=std::max(0,int(obs.own.shed[item])-protected_stock-existing[item]);
        if(obs.market.prices[item]>=2 && available[item]>0)priority[count++]=item;
    }
    std::sort(priority.begin(),priority.begin()+count,[&](int a,int b) {
        return obs.market.prices[a]!=obs.market.prices[b]?obs.market.prices[a]>obs.market.prices[b]:a<b;
    });
    bool changed=false;
    for(int i=0;i<count && shortfall>0;++i) {
        int item=priority[i],price=obs.market.prices[item];
        int quantity=std::min(available[item],int(std::floor((shortfall+price-1)/price)));
        if(quantity<=0)continue;
        int slot=-1;
        for(int m=0;m<action.n_orders;++m)if(action.orders[m].op==M_SELL && action.orders[m].item==item){slot=m;break;}
        if(slot>=0)action.orders[slot].n=std::max(0,action.orders[slot].n)+quantity;
        else if(action.n_orders<10)action.orders[action.n_orders++]={M_SELL,uint8_t(item),quantity};
        else continue;
        shortfall-=quantity*price;changed=true;
    }
    if(changed) {
        std::array<Order,10> orders{};int n=0;
        for(int m=0;m<action.n_orders;++m)if(action.orders[m].op==M_SELL)orders[n++]=action.orders[m];
        for(int m=0;m<action.n_orders;++m)if(action.orders[m].op!=M_SELL)orders[n++]=action.orders[m];
        std::copy_n(orders.begin(),n,action.orders);
    }
}
