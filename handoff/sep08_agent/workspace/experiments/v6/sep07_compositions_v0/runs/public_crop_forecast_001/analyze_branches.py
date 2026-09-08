"""Score observation-only branch rankings against previously realized controls."""
import csv
import argparse
import json
import statistics as s
from collections import defaultdict,Counter
from pathlib import Path
from branch_inputs import FAMILIES
from analyze_donors import NAMES

RUN=Path(__file__).resolve().parent


def main():
    global RUN
    parser=argparse.ArgumentParser();parser.add_argument('--directory',type=Path);args=parser.parse_args()
    if args.directory:RUN=args.directory.resolve()
    reports={};allcases=[]
    for family in FAMILIES:
        expected=json.loads((RUN/f'{family}_expected.json').read_text());predicted=json.loads((RUN/f'{family}_predictions.json').read_text());groups=defaultdict(list)
        assert len(expected)==len(predicted)
        for c,p in zip(expected,predicted):
            assert all(c[k]==p[k]for k in ['rival','seed','seat','own_cash','rival_cash','physical']),(family,c['seed'],c['seat'],c['rival'])
            for v in p['variants']:
                for objective in ['own','margin']:
                    scores=[x['own']-(x['rival']if objective=='margin'else 0)for x in v['branches']]
                    chosen=max(range(len(scores)),key=lambda i:scores[i]);margin=c['margins'][chosen];exact=c['own_final']if objective=='own'else c['margins'];best=max(exact)
                    key=v['integration'],v['crop_mode'],v['feed_net'],objective
                    baseline_estimate=v['branches'][0]['own']-v['branches'][0]['rival']
                    errors=[abs((x['own']-x['rival']-baseline_estimate)-(c['margins'][i]-c['margins'][0]))for i,x in enumerate(v['branches'])if i]
                    row={'family':family,'rival':c['rival'],'seed':c['seed'],'seat':c['seat'],'integration':key[0],'crop_mode':key[1],'feed_net':key[2],'objective':objective,'branch':chosen,'own_final':c['own_final'][chosen],'margin':margin,'win':margin>0,'tie':margin==0,'objective_regret':best-exact[chosen],'margin_regret':max(c['margins'])-margin,'margin_delta_error':s.mean(errors),'scores':scores,'branches':v['branches'],'actual_margins':c['margins']};groups[key].append(row);allcases.append(row)
        metrics=[]
        for (integration,mode,net,objective),rows in groups.items():
            metrics.append({'integration':integration,'crop_mode':mode,'mode_name':NAMES[mode],'feed_net':net,'objective':objective,'contexts':len(rows),'wins':sum(r['win']for r in rows),'ties':sum(r['tie']for r in rows),'mean_margin':s.mean(r['margin']for r in rows),'mean_own_cash':s.mean(r['own_final']for r in rows),'objective_best_choices':sum(r['objective_regret']==0 for r in rows),'objective_regret':s.mean(r['objective_regret']for r in rows),'margin_regret':s.mean(r['margin_regret']for r in rows),'margin_delta_error':s.mean(r['margin_delta_error']for r in rows),'branches':dict(Counter(r['branch']for r in rows))})
        for r in metrics:
            base=next(x for x in metrics if x['integration']==r['integration']and x['objective']==r['objective']and x['crop_mode']==0)
            r['win_delta_vs_same_integration_no_crops']=r['wins']-base['wins'];r['margin_delta_vs_same_integration_no_crops']=r['mean_margin']-base['mean_margin']
        reports[family]={'exact_prefix_physical_cash_matches':len(expected),'metrics':metrics,'top_margin_objective':sorted((r for r in metrics if r['objective']=='margin'),key=lambda r:(r['wins']+.5*r['ties'],r['mean_margin']),reverse=True)[:8]}
        with (RUN/f'{family}_ranking_metrics.csv').open('w')as f:w=csv.DictWriter(f,fieldnames=metrics[0]);w.writeheader();w.writerows(metrics)
    (RUN/'BRANCH_REPORT.json').write_text(json.dumps(reports,indent=2)+'\n');(RUN/'branch_cases.json').write_text(json.dumps(allcases,separators=(',',':'))+'\n')
    for family,r in reports.items():print(family,json.dumps(r['top_margin_objective'],indent=2))


if __name__=='__main__':main()
