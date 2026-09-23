#include "allocation.hpp"
#include <algorithm>
#include <cassert>

namespace kag::day_compiler {
SaleAllocation allocate_sales(const SaleCurve* curves,int count,int occupied,int capacity,int slots) {
    SaleAllocation result;
    if(count<0 || count>N_PRODUCTS || occupied<0 || capacity<occupied || capacity>100 || slots<0 || slots>10) return result;
    bool seen[N_PRODUCTS]{};
    for(int k=0;k<count;++k) {
        const auto& curve=curves[k];
        if(curve.product<0 || curve.product>=N_PRODUCTS || seen[curve.product] || curve.stock<0 || curve.stock>100 ||
           curve.maximum_buy<0 || curve.maximum_buy+curve.stock>100) return result;
        seen[curve.product]=true;
    }
    double current[11][101],next[11][101];
    int16_t parent[N_PRODUCTS][11][101]{};
    std::fill_n(&current[0][0],11*101,-1e100); current[0][occupied]=0;
    for(int k=0;k<count;++k) {
        const auto& curve=curves[k];
        std::fill_n(&next[0][0],11*101,-1e100);
        for(int orders=0;orders<=slots;++orders) for(int used=0;used<=capacity;++used) {
            if(current[orders][used]<-1e90) continue;
            for(int quantity=std::max(-curve.maximum_buy,used+curve.stock-capacity);quantity<=curve.stock;++quantity) {
                const int taken=orders+int(quantity!=0 && !(quantity>0 && curve.existing_sale_order)),space=used+curve.stock-quantity;
                const double continuation=quantity<0?curve.buy_value[-quantity]:curve.value[quantity];
                if(taken>slots || continuation<-1e90) continue;
                const double value=current[orders][used]+continuation;
                if(value>next[taken][space]+1e-9) {
                    next[taken][space]=value; parent[k][taken][space]=quantity;
                }
            }
        }
        std::copy_n(&next[0][0],11*101,&current[0][0]);
    }
    int best_orders=0,best_space=0;
    for(int orders=0;orders<=slots;++orders) for(int space=0;space<=capacity;++space)
        if(current[orders][space]>result.value+1e-9) {
            result.value=current[orders][space]; best_orders=orders; best_space=space;
        }
    if(result.value<-1e90) return result;
    for(int k=count-1;k>=0;--k) {
        const auto& curve=curves[k]; const int quantity=parent[k][best_orders][best_space];
        assert(quantity>=-curve.maximum_buy && quantity<=curve.stock);
        result.quantity[curve.product]=quantity;
        best_orders-=quantity!=0 && !(quantity>0 && curve.existing_sale_order); best_space-=curve.stock-quantity;
    }
    assert(!best_orders && best_space==occupied);
    result.feasible=true; return result;
}
}
