"""Separate current-crop forecast error from unknown future crop lifecycles."""
import csv
import hashlib
import json
import statistics as s
from pathlib import Path

RUN=Path(__file__).resolve().parent
NAMES=['none','latest_known_fert','cap_known_fert','cap_maintain_active_fert','cap_fully_fertilized','earliest_known_fert']
ITEMS=['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON']


def main():
    cases=json.loads((RUN/'donor_cases.json').read_text());predictions=json.loads((RUN/'donor_predictions.json').read_text());rows=[];coverage=[]
    for c,p in zip(cases,predictions):
        assert c['case']==p['case']
        current=c['current_crop_future_harvest'];full=c['all_crop_future_harvest']
        for product,name in enumerate(ITEMS):
            ct=sum(r[product]for r in current);ft=sum(r[product]for r in full);assert ft>=ct
            coverage.append({'item':name,'current':ct,'all_future':ft,'unknown_future_instances':ft-ct})
        for mode in [{'mode':0,'days':[[0]*5 for _ in range(30)]},*p['modes']]:
            for product,name in enumerate(ITEMS):
                q=[d[product]for d in mode['days']];truth=[d[product]for d in current];alltruth=[d[product]for d in full];sales=[d[product]for d in c['future_sales']];buys=[d[product]for d in c['future_buys']]
                rows.append({'case':c['case'],'item':name,'mode':NAMES[mode['mode']],'predicted':sum(q),'current_actual':sum(truth),'all_actual':sum(alltruth),'current_total_error':abs(sum(q)-sum(truth)),'current_daily_error':sum(abs(a-b)for a,b in zip(q,truth)),'all_total_error':abs(sum(q)-sum(alltruth)),'market_net_total_error':abs(sum(q)-sum(sales)+sum(buys))})
    metrics=[]
    for name in NAMES:
        for item in ITEMS:
            subset=[r for r in rows if r['mode']==name and r['item']==item]
            metrics.append({'mode':name,'item':item,'contexts':len(subset),**{k:s.mean(r[k]for r in subset)for k in ['predicted','current_actual','all_actual','current_total_error','current_daily_error','all_total_error','market_net_total_error']}})
    support=[]
    for item in ITEMS:
        a=[r for r in coverage if r['item']==item];current=sum(r['current']for r in a);total=sum(r['all_future']for r in a)
        support.append({'item':item,'current_actual':current,'all_future_actual':total,'fraction_from_current_visible_lives':current/total if total else None,'unknown_future_instance_output':total-current})
    summary=[]
    for name in NAMES:
        a=[r for r in metrics if r['mode']==name];summary.append({'mode':name,'current_total_absolute_error_per_context':sum(r['current_total_error']for r in a),'current_dated_absolute_error_per_context':sum(r['current_daily_error']for r in a),'all_future_total_absolute_error_per_context':sum(r['all_total_error']for r in a)})
    report={'scope':'504public observations,72globally selected top12games,7decision dates; repeated snapshots are not independent games. Forecasting current visible crop lifecycles only, no future shops/private state/replanting/animal expansion. Future observed data used only for scoring.','summary':summary,'coverage':support,'metrics':metrics,'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in [RUN/'source/crop_forecast.hpp',RUN/'source/donor_forecast.cpp',RUN/'donor_inputs.txt',RUN/'donor_cases.json',RUN/'donor_predictions.json']},'limitations':['Future harvested output is not necessarily sold, especially wheat fed internally','Same-day sales assume logistics and cash work; no exact route prediction','Known current crops cannot explain later new crops or intentional early removals','No branch-ranking or promotion claim yet']}
    (RUN/'DONOR_REPORT.json').write_text(json.dumps(report,indent=2)+'\n')
    with (RUN/'donor_metrics.csv').open('w')as f:w=csv.DictWriter(f,fieldnames=metrics[0]);w.writeheader();w.writerows(metrics)
    print(json.dumps({'summary':summary,'coverage':support},indent=2))


if __name__=='__main__':main()
