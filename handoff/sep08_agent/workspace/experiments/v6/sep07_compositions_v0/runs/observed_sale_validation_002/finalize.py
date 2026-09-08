"""Retain every outcome; promote only after all frozen validation gates pass."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]


def read(path):
    return json.loads(path.read_text())


def main():
    spec=read(RUN/'FRESH_PREREGISTERED.json')
    candidate,baseline=spec['candidate'],spec['baseline']
    fresh=read(RUN/'FRESH_ANALYSIS.json');native=read(RUN/'NATIVE_ANALYSIS.json')
    checks=read(RUN/'CHECKS.json');final=read(RUN/'FINAL_AUDITS.json');frozen=read(RUN/'frozen/FROZEN.json')
    for relative,expected in spec['candidate_files_sha256'].items():
        assert hashlib.sha256((EXP/relative).read_bytes()).hexdigest()==expected,relative
    for relative,expected in frozen['files_sha256'].items():
        assert hashlib.sha256((ROOT/relative).read_bytes()).hexdigest()==expected,relative
    gates={'fresh':all(fresh['gates'].values()),'native':all(native['gates'].values()),
           'operational':checks['generic_pair_debug_thread_all_records_equal'] and all(checks['sale_lead_changed_games'].values()),
           'frozen':final['frozen_full_records_equal'] and final['active_branch_changed_games']>0}
    promoted=all(gates.values());now=datetime.now(timezone.utc).isoformat()
    report={'completed_utc':now,'candidate':candidate,'parent':baseline,'status':'Promoted experimental reference after all frozen gates.' if promoted else 'Not promoted: at least one required frozen gate failed.',
            'acceptance':gates,'fresh':fresh,'native_and_pass':native,'operational':checks,'frozen':{'path':str((RUN/'frozen/FROZEN.json').relative_to(EXP)),'dependencies':len(frozen['files_sha256']),'full_records_exact':final['frozen_games']},
            'lineage':'runs/observed_sale_lead_004/LINEAGE.json','donor':'AhmedV23 sale-lead/projected-shed/suppression concepts; exact public source lineage in league/ahmed_v23/IMPORT.json.',
            'local_changes':['Forecast next raw sale requests from independently copied current policy and a time-advanced current observation.','Share six immutable calendar vectors in a separately namespaced parent snapshot; retain private mutable episode state.','Start at216 after the initial farm-funding phase, selected from four global times after diagnosing a Junghoon regression.'],
            'scope':'Retains all inherited shop/animal/crop/whole-farm branches. Adds earlier non-input sales where own stock and a predicted next-turn request permit; skips current consumption and day boundaries.',
            'discovery':'runs/observed_sale_lead_004/EXTENDED_ANALYSIS.json','failed_predecessor':'results/observed_sale_lead_v3_validation.json','causal_witness':'runs/observed_sale_lead_003/JUN_FINANCING_CAUSE.json',
            'limits':['Forecast accuracy is empirically verified, not a proof of future-state prediction for arbitrary policies.','The global start-time guard preserves initial funding; it is not a complete strategic model of rival capital decisions.','No pairwise dominance claim against every unpromoted specialist. No Kaggle ranking inferred from local games.','General competitive construction from arbitrary compositions and layouts remains incomplete.','No new official catalog copy, Git operation or Kaggle submission.']}
    if promoted:report['promoted_utc']=now
    target=EXP/'results/observed_sale_lead_start216_validation.json'
    target.write_text(json.dumps(report,indent=2)+'\n')
    if not promoted:
        print(report['status'],gates);return
    current=read(EXP/'CURRENT_REFERENCE.json');assert current['name']==baseline
    (RUN/'PREVIOUS_REFERENCE.json').write_text(json.dumps(current,indent=2)+'\n')
    current.update(name=candidate,path='runs/observed_sale_lead_004/proposals/'+candidate,previous=baseline,promoted_utc=now,report=str(target.relative_to(EXP)))
    (EXP/'CURRENT_REFERENCE.json').write_text(json.dumps(current,indent=2)+'\n')
    summary=f"{now}: Promoted {candidate}. Fresh{fresh['games']}games/32opponents, native/PASS{native['games']}, operational{checks['games']}, frozen{final['frozen_games']} full records/{len(frozen['files_sha256'])} C++ dependencies pass. Advance non-input sales after step216 using copied-policy prediction; preserve initial farm funding. Read {target.relative_to(EXP)}. No upload or Git."
    for name in ['LINEAGE.md','IDEAS_LEDGER.md','PROFILING_LEDGER.md','PROGRESS.md','OBJECTIVE_COVERAGE.md']:
        with (EXP/name).open('a') as f:f.write('\n\n'+summary+'\n')
    path=EXP/'README.md';text=path.read_text();start=text.index('Current broad search starting point');end=text.index('Previous reference (21:49 UTC):',start)
    block=f'''Current broad search starting point ({now[11:16]} UTC):
`{current['path']}/`.
Keep the inherited adaptive farm and advance eligible non-input product sales
after step216. A copied policy predicts the next sale from current public/own
state; immutable calendars share storage. The initial funding phase is preserved
after an earlier-sale prototype accidentally improved a rival's farm financing.
Fresh{fresh['games']} games against32 opponents, native/PASS{native['games']},
{checks['games']} operational games and{final['frozen_games']} frozen rebuilt full
records pass all declared gates. Read `CURRENT_REFERENCE.json` and
`{target.relative_to(EXP)}` for metrics, source lineage and limitations.

Previous reference (22:49 UTC):
`runs/rival_wool_context_003/proposals/rival_wool_context_v3/`;
`results/rival_wool_context_v3_validation.json` retains its evidence.

'''
    path.write_text(text[:start]+block+text[end:])
    print(summary);print('Current league',fresh['groups']['current']);print('Direct parent',fresh['metrics'][candidate][baseline])


if __name__=='__main__':main()
