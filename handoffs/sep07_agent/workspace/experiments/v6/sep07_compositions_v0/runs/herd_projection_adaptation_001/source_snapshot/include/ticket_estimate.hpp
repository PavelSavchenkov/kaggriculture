#pragma once
#include "ticket_trace.hpp"

namespace compositions {
inline Biology ticket_biology(const AnimalTicket& t,int item,bool attempts) {
    Service service;service.water=0;service.fertilize=0;
    service.feed=t.feed;service.care=t.care;service.collect_fertilizer=t.collect;
    service.harvest=attempts?t.harvest_requested:t.harvest;
    return biology({uint8_t(item),1,t.start_day,t.end_day},service);
}

struct TicketEstimate {
    MarketValue value;
    int removed_unmatched=0,added_unsold=0;
};

inline TicketEstimate estimate_ticket(const TicketTrace& trace,const AnimalTicket& ticket,int replacement) {
    using namespace kag;
    TicketEstimate result;auto market=trace.market;
    const int seat=trace.seat,from=ANIMALS[ticket.edit.original-GOOSE].product,to=ANIMALS[replacement-GOOSE].product;
    const auto original=ticket_biology(ticket,ticket.edit.original,false),changed=ticket_biology(ticket,replacement,true);
    market[ticket.edit.purchase.step].slots[ticket.edit.purchase.index].fixed[seat]+=changed.animal_cost-original.animal_cost;
    for(int day=0;day<30;++day) {
        const int harvest=ticket.harvest_step[day];
        int remove=original.days[day].output[from],add=changed.days[day].output[to];
        if(harvest<0 && (remove || add))std::abort();
        // One turn is a relaxed deposit delay. Preserve available source sale
        // slots and report unmatched output; exact routes and order caps remain
        // downstream compiler uncertainty.
        for(int step=harvest+1;step<719 && remove>0;++step)
            for(auto& slot:market[step].slots) {
                auto& trade=slot.trades[seat];
                if(trade.op==M_SELL && trade.item==from) {
                    const int take=std::min(remove,trade.n);trade.n-=take;remove-=take;
                }
            }
        for(int step=harvest+1;step<719 && add>0;++step) {
            int index=trace.sale_slots[step][to];
            auto& turn=market[step];
            if(index<0)for(int i=trace.market[step].order_count[seat];i<turn.order_count[seat];++i) {
                const auto& trade=turn.slots[i].trades[seat];
                if(trade.op==M_SELL && trade.item==to)index=i;
            }
            if(index<0 && trace.sale_slots[step][from]>=0 && turn.order_count[seat]<10) {
                index=turn.order_count[seat]++;
                turn.slots[index].trades[seat]={M_SELL,to,0};
            }
            if(index>=0) {
                auto& trade=turn.slots[index].trades[seat];
                if(trade.op!=M_SELL || trade.item!=to)std::abort();
                trade.n+=add;add=0;
            }
        }
        result.removed_unmatched+=remove;result.added_unsold+=add;
    }
    result.value=value_market(market,trace.shops);return result;
}
}
