"""Verify the bounded research deliverables and record the final accepted state."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RESEARCH=Path(__file__).resolve().parent
EXP=RESEARCH.parent
ROOT=EXP.parents[2]


def read(path):
    return json.loads(path.read_text())


current=read(EXP/'CURRENT_REFERENCE.json')
assert current['name']=='observed_sale_lead_start_216'
report=read(EXP/current['report'])
run=EXP/'runs/observed_sale_validation_002'
spec=read(run/'FRESH_PREREGISTERED.json')
amendment=read(run/'PACKAGING_AMENDMENT.json')
assert all(report['acceptance'].values())
assert report['fresh']['games']==65536
assert report['native_and_pass']['games']==5632
assert report['operational']['games']==1024
assert report['frozen']['full_records_exact']==64
assert all(m['utility']>.5 for m in report['fresh']['metrics'][current['name']].values())
for path,expected in spec['candidate_files_sha256'].items():
    if path==amendment['path']:
        assert expected==amendment['before_sha256'];expected=amendment['after_sha256']
    assert hashlib.sha256((EXP/path).read_bytes()).hexdigest()==expected,path
frozen=read(run/'frozen/FROZEN.json')
for path,expected in frozen['files_sha256'].items():
    assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==expected,path

cold=read(EXP/'runs/search_v0_001/summary.json')
assert cold['estimated']==583 and cold['exact_games']==288
portfolio=read(EXP/'results/late_portfolio_validation.json')
assert all(portfolio['confirmation']['gates'].values()) and portfolio['confirmation']['games']==180224
counterfactual=read(EXP/'runs/late_portfolio_001/COUNTERFACTUAL_ANALYSIS.json')
assert counterfactual['games']==8192 and counterfactual['prediction_prefixes_equal']
refresh=read(RESEARCH/'refresh_0008/REFRESH.json')
assert refresh['player_games']==72
reviews=[RESEARCH/f'review_{i}.md' for i in range(64,70)]
assert all(p.is_file() for p in reviews)
assert len(read(RESEARCH/'review_69_comparison.json'))==82
required_files=['include/biology.hpp','include/estimate.hpp','include/economics.hpp','src/search_compositions.cpp',
                'candidates/composition_greedy_v0/source/agent.cpp','scheduler/README.md','runs/late_portfolio_001/source/model.hpp',
                'runs/late_portfolio_001/source/policy.hpp','runs/v52_family_001/DAY_LIBRARY.json','LINEAGE.md','IDEAS_LEDGER.md',
                'PROFILING_LEDGER.md','OBJECTIVE_COVERAGE.md','research/reproduction_entry.md']
assert all((EXP/p).is_file() for p in required_files)
hashes={p:hashlib.sha256((EXP/p).read_bytes()).hexdigest() for p in required_files}
snapshot={'closed_utc':datetime.now(timezone.utc).isoformat(),'research_window_end_utc':'2026-09-08T00:47:00+00:00',
          'closure_note':'No new strategy hypotheses after the window. The already-running fresh validation completed and passed during final closure.',
          'accepted_reference':current,'acceptance':report['acceptance'],'fresh_games':65536,'native_pass_games':5632,'operational_games':1024,
          'frozen_records':64,'frozen_cpp_dependencies':len(frozen['files_sha256']),
          'research_deliverables':{
              'dated_composition_and_fast_estimator':'Implemented and tested; full dated biology/economics, greedy placement and approximate labor exist. Accuracy is strongest within known executable alternatives.',
              'composition_estimate_compile_feedback':'Four-way day13 crop/goose/cow/sheep loop;8192 counterfactual games, corrected labor costs and180224-game independent confirmation.',
              'daily_scheduling_and_local_changes':'Checked V30 schedules, complete dependency rebuilding, crop replacements, whole-farm transfer and market timing experiments have exact-game evidence.',
              'warm_and_cold_search':'Independent outer loop estimated583 proposals and played288 exact games. Negative result retained; best had0win utility against its old public-router opponent.',
              'growing_league_and_self_play':'Final32-opponent panel includes teammate, public ports, replay strategies, accepted parents and rejected specialists; every tested opponent has candidate win utility above0.5.',
              'replay_notebook_learning_and_lineage':'Repeated fresh cohorts, source-parity-checked ports, copied schedules and component ablations retained with origins and hashes; latest72 player-games and three-notebook audit complete.',
              'reviews_and_fidelity':'Original objective coverage retained; numbered reviews include82 global/local metrics, failures and explicit pivots.',
              'reproduction_and_final_agent':'Accepted C++ package, full results, commands, isolated237-dependency rebuild, source hashes and reproduction entry verified.'},
          'remaining_research_limits':[
              'General competitive construction from arbitrary dated compositions and new layouts is not established; the independent greedy compiler remains weak.',
              'Labor, intraday liquidity, rival crop flows and future-demand errors limit valuation outside known executable alternatives.',
              'No claim of the globally strongest Kaggle controller or pairwise dominance over every unpromoted specialist. Top-player controllers are not all available.',
              'Final900000 reserve remains unused for future work. The accepted local agent has not been packaged or submitted to Kaggle.'],
          'source_sha256':hashes}
(RESEARCH/'final_audit.json').write_text(json.dumps(snapshot,indent=2)+'\n')
metrics=report['fresh']['metrics'][current['name']]
text=f'''# Final research result

The24-hour research window ended at00:47UTC. The final launched validation
completed during closure. The accepted local agent is
`{current['name']}` at `../{current['path']}/`.

It adds sale advancement after the initial farm-funding phase to the inherited
adaptive crop/animal policy. It predicts the next sale using current public/own
observations and an independent policy copy, with immutable calendars shared.
An earlier version was rejected because reducing a rival's early revenue
accidentally improved its later farm. The failure and fix are preserved.

| Fresh opponent | Wins / games | Mean margin |
| --- | ---: | ---: |
'''
for opponent in [current['previous'],current['last_submission'],'teammate_shoprouter','ahmed_v23','public_router_v52','public_router_v5']:
    m=metrics[opponent];text+=f"| {opponent} | {m['wins']}/{m['games']} | ${m['mean_margin']:,.2f} |\n"
text+='''
The65,536-game paired panel passes every declared gate across32opponents.
Every opponent's win utility, mean margin and worst-decile margin nondecrease.
The current grouped utility rises90.710% to94.697%;95% paired gain interval
+3.739 to+4.243 percentage points. The historical grouping rises95.058% to
95.747%. Compare these within this panel; earlier promotion groupings differ.

All5,632native/PASSgames,1,024operationalgames and64isolated rebuilt records
pass. In the native panel, own production, hires and failed actions are unchanged;
this final component earns through timing and shared prices. The inherited
lineage also contains actual crop, animal, whole-farm and labor improvements.

The composition intuition worked best when comparing complete known
alternatives: the four-way late investment selector uses cheap economics,
compiled labor costs and sampled future shops, then verifies actual execution.
The general greedy compiler was tested and remained weak. Accurate valuation
and competitive construction of unfamiliar farms are the largest remaining
research problems; they are not claimed solved by this result.

Read `final_audit.json`, `../CURRENT_REFERENCE.json`, the linked central
validation report, `reproduction_entry.md`, and the source/idea ledgers for the
evidence and continuation path. The earlier lineage snapshot remains unchanged.
The committed catalog agent and last submitted agent remain separate versions.
No Git operation, official catalog update or additional submission was made
during this final continuation.
'''
text=text.replace('The24-hour','The 24-hour').replace('at00:47UTC','at 00:47 UTC').replace('The65,536-game','The 65,536-game').replace('across32opponents','across 32 opponents').replace('rises90.710% to94.697%;95%','rises 90.710% to 94.697%; 95%').replace('rises95.058% to','rises 95.058% to').replace('All5,632native/PASSgames,1,024operationalgames and64isolated rebuilt records','All 5,632 native/PASS games, 1,024 operational games and 64 isolated rebuilt records')
(RESEARCH/'final_result.md').write_text(text)
print('Final research artifacts and current accepted C++ source hashes verified.')
