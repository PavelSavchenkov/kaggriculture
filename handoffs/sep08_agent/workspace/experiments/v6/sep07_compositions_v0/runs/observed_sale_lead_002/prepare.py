"""Prune impossible sale-lead forecasts without changing their market actions."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
OLD=EXP/'runs/observed_sale_lead_001'
mapping={'sale_lead_prediction_control':'sale_lead_fast_control','observed_sale_lead_v1':'observed_sale_lead_fast_v2',
         'observed_sale_lead_milk_wool':'observed_sale_lead_fast_milk_wool','compositions::observed_sale_lead':'compositions::observed_sale_lead_fast'}


def renamed(text):
    for old,new in mapping.items():text=text.replace(old,new)
    return text


(RUN/'source').mkdir(exist_ok=True)
policy=renamed((OLD/'source/policy.hpp').read_text())
needle='        Base shadow=base_;Obs next=o;'
guard='''        Stock available=projected(o,a);bool already[kag::N_ITEMS]{};
        for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_SELL)already[a.orders[i].item]=true;
        bool possible=false;
        for(int item=1;item<kag::FERTILIZER;++item)
            possible|=available[item]>0 && !already[item] && o.market.prices[item]>=2 &&
                (mode_!=2 || item==kag::MILK || item==kag::WOOL);
        if(a.n_orders>=10 || !possible){a.finalize();return;}
'''
assert policy.count(needle)==1
duplicate='''        Stock available=projected(o,a);bool already[kag::N_ITEMS]{};
        for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_SELL)already[a.orders[i].item]=true;
'''
assert policy.count(duplicate)==1
policy=policy.replace(duplicate,'').replace(needle,guard+needle)
(RUN/'source/policy.hpp').write_text(policy)
(RUN/'register.py').write_text(renamed((OLD/'prepare.py').read_text()))
subprocess.run(['conda','run','-n','kaggriculture','python',str(RUN/'register.py')],check=True)
for name in ['run_discovery.py','forecast_probe.cpp','run_probe.py']:
    (RUN/name).write_text(renamed((OLD/name).read_text()))
report={'created_utc':datetime.now(timezone.utc).isoformat(),'parent':'observed_sale_lead_v1',
        'source_sha256':hashlib.sha256((OLD/'source/policy.hpp').read_bytes()).hexdigest(),
        'new_sha256':hashlib.sha256((RUN/'source/policy.hpp').read_bytes()).hexdigest(),
        'reason':'Completed256game prototype takes140.068seconds; parent9.46499. Skip copies when full market or no eligible available product; predicate implies previous sale-lead loop cannot add an order.',
        'required_parity':'All completed full prototype game records must match fast equivalent. Fresh validation only after parity and broad screen.',
        'limits':'No change to approximate next-observation model; only fewer fruitless calls. Diagnostic forecast counters intentionally count eligible opportunities only.'}
(RUN/'OPTIMIZATION.json').write_text(json.dumps(report,indent=2)+'\n')
print('Fast source copy registered; original slow prototype preserved.')
