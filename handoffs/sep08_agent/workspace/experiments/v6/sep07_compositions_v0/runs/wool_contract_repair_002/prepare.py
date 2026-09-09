"""Reuse the missed-order repair wherever the exact donor day contract runs."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
OLD = EXP / 'runs/rival_wool_repair_001'
NAME = 'wool_contract_repair_v2'
source = (OLD / 'source/repair.hpp').read_text()
source = source.replace('namespace compositions::rival_wool_repair', 'namespace compositions::wool_contract_repair')
source = source.replace('    int expected_=0', '    bool active_=false;\n    int capacity_=100;\n    int expected_=0')
source = source.replace('    static int sheep_count', '''    static bool same_action(const kag::Action& a,const kag::Action& b) {
        if(a.n_units!=b.n_units || a.n_orders!=b.n_orders)return false;
        for(int i=0;i<a.n_units;++i)if(a.units[i].op!=b.units[i].op || a.units[i].arg!=b.units[i].arg || a.units[i].n!=b.units[i].n)return false;
        for(int i=0;i<a.n_orders;++i)if(a.orders[i].op!=b.orders[i].op || a.orders[i].item!=b.orders[i].item || a.orders[i].n!=b.orders[i].n)return false;
        return true;
    }
    static int sheep_count''')
source = source.replace('base_.reset(init);expected_=', 'base_.reset(init);active_=false;capacity_=init.config.shed_capacity;expected_=')
source = source.replace('rival_wool_purchase_repair_v1', NAME)
before = '        if(!base_.selected() || o.step<217 || o.step>253)return;'
after = '''        if(o.step==216) {
            active_=base_.selected();
            for(const auto& g:v52_family::improved_days())if(g.plan.day==9 && g.matches(o) && same_action(a,g.plan.actions[0]))active_=true;
        }
        if(!active_)return;
        if(o.step==263 && a.n_orders==0) {
            bool stable_wheat=true;
            for(int u=0;u<a.n_units;++u)stable_wheat &= a.units[u].op!=kag::OP_FEED && a.units[u].op!=kag::OP_HARVEST;
            int wheat=o.own.shed[kag::WHEAT],total=o.own.shed_total;
            for(int u=0;u<o.self().n_units;++u)for(int item=0;item<kag::N_ITEMS;++item) {
                total+=o.own.inv[u][item];if(item==kag::WHEAT)wheat+=o.own.inv[u][item];
            }
            for(const auto& g:v52_family::improved_days())if(g.plan.day==11 && stable_wheat && total<capacity_ &&
                    wheat==g.shed[kag::WHEAT]-1 && o.self().money>=kag::market_price(kag::WHEAT,o.market.inventory[kag::WHEAT]-1)) {
                a.orders[a.n_orders++]={kag::M_BUY_PRODUCT,kag::WHEAT,1};a.finalize();break;
            }
            return;
        }
        if(o.step<217 || o.step>253)return;'''
assert source.count(before) == 1
source = source.replace(before, after).replace('100-int(o.own.shed_total)', 'capacity_-int(o.own.shed_total)')
(RUN / 'source').mkdir(exist_ok=False)
(RUN / 'source/repair.hpp').write_text(source)
package = RUN / 'proposals' / NAME
(package / 'source').mkdir(parents=True)
manifest = json.loads((OLD / 'proposals/rival_wool_purchase_repair_v1/agent.json').read_text())
manifest.update(name=NAME, type=f'kag::agents::{NAME}::Agent')
(package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
(package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/repair.hpp"\n'
    f'namespace kag::agents::{NAME}{{using Agent=compositions::wool_contract_repair::Policy;}}\n')
(package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
(package / 'README.md').write_text('''# Wool contract repair v2

Experimental, not promoted. Preserve rival_wool_context_v3. Recognize the exact
donor day9 plan and physical contract, including existing early wool selections.
Retry its partially funded sheep order before pickup253. At263, buy one missing
wheat for the day11 starting contract only when actions preserve portable wheat,
no other market order is scheduled, cash suffices and all carried goods fit.

No worker routes or animal choices are added. Full-game validation is pending.
Source, preceding experiment and known assumptions are in IMPORT.json.
''')
origin = json.loads((OLD / 'proposals/rival_wool_purchase_repair_v1/IMPORT.json').read_text())
origin.update(previous_candidate='rival_wool_purchase_repair_v1',
              idea='Recognize exact executing day contracts; honor their animal pickup and next-day wheat obligations.',
              scope='Both early and delayed wool families when the exact day9 physical and action contract matches.',
              parity='Preceding repair discovery2560games, no paired native margin loss. New contract expansion and wheat repair need validation.',
              policy_sha256=hashlib.sha256((RUN / 'source/repair.hpp').read_bytes()).hexdigest())
(package / 'IMPORT.json').write_text(json.dumps(origin, indent=2) + '\n')
registry = json.loads((EXP / 'configs/league.json').read_text())
assert NAME not in registry
registry[NAME] = str(package.relative_to(ROOT))
(EXP / 'configs/league.json').write_text(json.dumps(registry, indent=2) + '\n')
print('Prepared', NAME)
