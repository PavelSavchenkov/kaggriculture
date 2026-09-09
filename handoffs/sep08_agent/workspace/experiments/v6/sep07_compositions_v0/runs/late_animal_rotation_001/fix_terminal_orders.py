from pathlib import Path

RUN=Path(__file__).resolve().parent
path=RUN/'source/compile.cpp'
text=path.read_text()
(RUN/'source/compile_initial.cpp.txt').write_text(text)
text=text.replace('    std::vector<TileWorkAction> result;\n    auto add=', '    if(r.item<0)return original;\n    std::vector<TileWorkAction> result;\n    auto add=')
text=text.replace('if(day<last-1)add(OP_CARE);', 'if(last<=29 && day<last-1)add(OP_CARE);')
text=text.replace('!is_animal(rotation.item)||seconds<=0', '(rotation.item!=-1 && !is_animal(rotation.item))||seconds<=0')
needle='    std::cerr<<"no market slot: hours="'
start=text.index(needle)
text=text[:start]+'''    // Accepted source actions retain empty slots. Reuse those before
    // rejecting a new sale, then merge a same-product sale if the hour is full.
    for(int h=first;h<=last;++h)for(int s=0;s<actions[h].n_orders;++s)
        if(actions[h].orders[s].op==M_NONE) {
            actions[h].orders[s]={uint8_t(op),uint8_t(item),n};actions[h].finalize();return true;
        }
    if(op==M_SELL)for(int h=first;h<=last;++h)for(int s=0;s<actions[h].n_orders;++s) {
        auto& order=actions[h].orders[s];
        if(order.op==M_SELL && order.item==item){order.n+=n;actions[h].finalize();return true;}
    }
''' + text[start:]
path.write_text(text)
