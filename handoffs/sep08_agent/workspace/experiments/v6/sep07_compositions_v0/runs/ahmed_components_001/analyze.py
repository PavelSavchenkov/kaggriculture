"""Attribute full-controller gains to separately disabled runtime layers."""
from pathlib import Path
import hashlib
import json
import math
import statistics

RUN=Path(__file__).resolve().parent
variants=json.loads((RUN/'LINEAGE.json').read_text())['variants']


def metrics(games):
    margins=[g['cash']-g['opponent_cash'] for g in games]
    return {'games':len(games),'wins':sum(v>0 for v in margins),'ties':sum(v==0 for v in margins),
            'utility':statistics.mean((v>0)+.5*(v==0) for v in margins),'margin':statistics.mean(margins),
            'tail':statistics.mean(sorted(margins)[:math.ceil(len(games)/10)]),
            'cash':statistics.mean(g['cash'] for g in games),'rival_cash':statistics.mean(g['opponent_cash'] for g in games),
            'faults':statistics.mean(g['unit_faults'] for g in games),
            'hires':statistics.mean(g['profile']['hires'] for g in games),'hire_cost':statistics.mean(g['profile']['hire_cost'] for g in games),
            'discards':statistics.mean(sum(g['discarded']) for g in games)}


def main():
    report={'scope':'128discoverygames/opponent; component attribution, not promotion. Negative delta means disabling the layer hurts.',
            'comparisons':{},'full_control_records_exact':0,'files_sha256':{}}
    for path in sorted((RUN/'discovery').glob('ahmed_v23_vs_*.json')):
        opponent=path.stem.split('_vs_',1)[1]
        base=json.loads(path.read_text())['games'];base_metrics=metrics(base);rows={}
        for variant in variants:
            other=path.parent/f'{variant}_vs_{opponent}.json'
            games=json.loads(other.read_text())['games'];values=metrics(games)
            assert [(g['seed'],g['seat']) for g in games]==[(g['seed'],g['seat']) for g in base]
            if variant=='ahmed_layers_all':
                assert games==base
                report['full_control_records_exact']+=len(games)
            delta={k:values[k]-base_metrics[k] for k in values if k!='games'}
            row={'metrics':values,'delta':delta,'changed':sum(x!=y for x,y in zip(games,base)),
                 'own_production_delta':[statistics.mean(x['produced'][i]-y['produced'][i] for x,y in zip(games,base)) for i in range(9)],
                 'rival_actions_changed':sum(x['opponent_action_hash']!=y['opponent_action_hash'] for x,y in zip(games,base))}
            rows[variant]=row
            report['files_sha256'][str(other.relative_to(RUN))]=hashlib.sha256(other.read_bytes()).hexdigest()
            print(opponent,variant,'wins',delta['wins'],'margin',round(delta['margin'],2),'own',round(delta['cash'],2),'faults',round(delta['faults'],2),'changed',row['changed'],flush=True)
        report['comparisons'][opponent]={'baseline':base_metrics,'variants':rows}
    assert len(report['comparisons'])==6
    report['aggregate']={v:{k:statistics.mean(r['variants'][v]['delta'][k] for r in report['comparisons'].values()) for k in ['utility','margin','cash','rival_cash','faults','hires','discards']} for v in variants}
    (RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
    print('AGGREGATE',json.dumps(report['aggregate']),flush=True)


if __name__=='__main__':main()
