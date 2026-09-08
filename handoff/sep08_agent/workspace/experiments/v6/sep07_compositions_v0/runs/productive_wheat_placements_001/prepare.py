"""Rank semantic placements and preserve the exact established positive control."""
import csv
import hashlib
import json
import shutil
from pathlib import Path

RUN=Path(__file__).resolve().parent
SOURCE=RUN.parent/'productive_wheat_rotation_001'


def main():
    inventory=RUN/'inventory'
    estimates={int(p.stem.split('_')[1]):json.loads(p.read_text())for p in sorted(inventory.glob('estimate_*.json'))}
    assert sorted(estimates)==[13,21,22,31,40,80]
    visits=list(csv.DictReader((inventory/'visits.csv').open()))
    pairs=[(a,b)for a in estimates for b in estimates if a<b and abs(a%10-b%10)+abs(a//10-b//10)==1]
    assert pairs==[(21,22),(21,31)]
    proposals=[]
    for cells in [(c,)for c in estimates]+pairs:
        name='wp_c'+str(cells[0])if len(cells)==1 else'wp_p'+'_'.join(map(str,cells))
        value=sum(estimates[c]['shadow_cash_delta']for c in cells)
        deficit=sum(int(v['visit_deficit'])for v in visits if int(v['cell'])in cells)
        proposals.append({'name':name,'cells':list(cells),'first':13,'cycles':3,'fertilizer':True,
                          'frozen_control':cells==(22,), 'source_quote_value':value,
                          'missing_flexible_visits':deficit,
                          'field_operations_delta':sum(estimates[c]['field_operations_delta']for c in cells),
                          'output_delta':[sum(estimates[c]['output_delta'][i]for c in cells)for i in range(9)],
                          'fertilizer_delta':3*len(cells),
                          'ranking_score':value-8*deficit,
                          'ranking_scope':'Source-quote delta minus heuristic$8 per missing visit; not a bound or a worker cost prediction'})
    proposals.sort(key=lambda p:p['ranking_score'],reverse=True)
    (RUN/'PROPOSALS.json').write_text(json.dumps(proposals,indent=2)+'\n')
    hashes={}
    for suffix in ['','_berry']:
        old=SOURCE/('one_fert'+suffix);target=RUN/('wp_c22'+suffix)
        assert not target.exists()
        shutil.copytree(old,target)
        for p in sorted(target.rglob('*')):
            if not p.is_file():continue
            hashes[str(p.relative_to(RUN))]=hashlib.sha256(p.read_bytes()).hexdigest()
            if p.name=='schedule.hpp':p.write_text(p.read_text().replace('one_fert'+suffix+'_d','wp_c22'+suffix+'_d'))
    (RUN/'CONTROL_ORIGIN.json').write_text(json.dumps({'source_run':str(SOURCE),'raw_copied_sha256':hashes,
          'change':'Only schedule C++ namespaces renamed one_fert[_berry]_d to wp_c22[_berry]_d; action/guard/problem files unchanged'},indent=2)+'\n')
    print(json.dumps(proposals,indent=2))


if __name__=='__main__':main()
