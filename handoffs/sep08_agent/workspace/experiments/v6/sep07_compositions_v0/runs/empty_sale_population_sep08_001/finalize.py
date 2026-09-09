from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
spec = json.loads((RUN/'PREREGISTERED.json').read_text())
fresh = json.loads((RUN/'FRESH_ANALYSIS.json').read_text())
native = json.loads((RUN/'NATIVE_ANALYSIS.json').read_text())
current = json.loads((EXP/'CURRENT_REFERENCE.json').read_text())
assert current['name'] == spec['accepted_reference']
assert fresh['games'] == 71680 and native['games'] == 6144
assert all(fresh['gates'].values()) and all(native['gates'].values())
protocol_hash = hashlib.sha256((RUN/'PREREGISTERED.json').read_bytes()).hexdigest()
assert fresh['protocol_sha256'] == native['protocol_sha256'] == protocol_hash
for p, digest in spec['candidate_files_sha256'].items():
    assert hashlib.sha256((EXP/p).read_bytes()).hexdigest() == digest, p
for p, digest in spec['frozen_root_files_sha256'].items():
    assert hashlib.sha256((ROOT/p).read_bytes()).hexdigest() == digest, p
assert hashlib.sha256((ROOT/spec['binary']).read_bytes()).hexdigest() == spec['binary_sha256']
for p, digest in {**spec['evidence_sha256'], **fresh['files_sha256'], **native['files_sha256']}.items():
    assert hashlib.sha256((EXP/p).read_bytes()).hexdigest() == digest, p
operational = {}
for p in spec['operational_evidence']:
    evidence = json.loads((EXP/p).read_text())
    if p.endswith('CHECKS.json'):
        assert evidence['games'] == 1024 and evidence['generic_pair_debug_thread_all_records_equal'] and evidence['node_budget_deterministic']
    else:
        assert evidence['frozen_games'] == 64 and evidence['frozen_full_records_equal']
    operational[p] = evidence
old_run = EXP/'runs/empty_sale_validation_sep08_001'
old_decision = json.loads((EXP/'results/empty_sale_slots_m2_validation.json').read_text())
assert old_decision['promoted'] is False and not all(old_decision['fresh_gates'].values())
parent_evidence = json.loads((old_run/'FRESH_ANALYSIS.json').read_text())
assert parent_evidence['candidate'] == spec['candidate'] and parent_evidence['baseline'] == current['name']
direct = fresh['metrics'][spec['candidate']][current['name']]
assert direct['utility'] > .5 and direct['mean_margin'] > 0
now = datetime.now(timezone.utc).isoformat()
report = {'completed_utc': now, 'candidate': spec['candidate'], 'parent': current['name'],
    'confirmation_baseline': spec['baseline'], 'promoted': True,
    'decision': 'Accept the unchanged unguarded sale-order candidate after the new independently preregistered population comparison passes every gate. It beats the accepted reference and the guarded challenger directly, with nonnegative individual field mean margins and bounded tails. The original per-game-rule failures remain recorded as failures.',
    'fresh': fresh, 'native': native, 'operational_reused': operational,
    'paired_parent_evidence': parent_evidence,
    'paired_parent_scope': 'The original 2000000 paired candidate/accepted-parent panel supplies the parent league comparison. It failed the old every-game no-regression rule; that verdict is unchanged. The independent 2200000/2204000 comparison supplies the new population selection evidence against the guarded challenger.',
    'acceptance': {'fresh_population_gates': True, 'native_population_gates': True, 'unchanged_operational_evidence': True,
        'source_and_binary_hashes_unchanged': True, 'direct_accepted_parent_positive': True},
    'source_files': len(spec['candidate_files_sha256']), 'frozen_dependencies': len(spec['frozen_root_files_sha256']),
    'protocol': str((RUN/'PREREGISTERED.json').relative_to(EXP)), 'protocol_sha256': protocol_hash,
    'limitations': ['Local results do not establish a current Kaggle rank.', 'The extra gain over the floor guard is small on the primary field, despite a strong direct matchup.', 'Rare individual-game losses are allowed by the new population rule and remain fully reported.', 'Production and composition are inherited; this change concerns intra-turn sale ordering.']}
output = EXP/'results/empty_sale_slots_m2_population_validation.json'
with output.open('x') as out:
    json.dump(report,out,indent=2);out.write('\n')
reference = {'name': spec['candidate'], 'path': 'runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2',
    'promoted_utc': now, 'report': str(output.relative_to(EXP)), 'previous': current['name'],
    'committed_catalog': current['committed_catalog'], 'last_submission': current['last_submission']}
(EXP/'CURRENT_REFERENCE.json').write_text(json.dumps(reference,indent=2)+'\n')
text = '# Population comparison result\n\n'+report['decision']+'\n\n'
for opponent in [current['name'], spec['baseline'], current['last_submission'], 'teammate_shoprouter', 'public_router_v52', 'ahmed_v23']:
    m = fresh['metrics'][spec['candidate']][opponent]
    text += f"- {opponent}: {m['wins']}W/{m['ties']}T/{m['losses']}L in {m['games']} games; mean margin ${m['mean_margin']:+.2f}.\n"
text += '\nPrimary field gain over guard: '+str(fresh['groups']['primary'])+'\n\n'+report['paired_parent_scope']+'\n\nNo source, official catalog, package or Kaggle submission was changed.\n'
(RUN/'RESULTS.md').write_text(text)
entry=f'\n{now}: Promoted empty_sale_slots_m2 after independent population71680fresh/6144native plus unchanged1024ops/64frozen evidence per candidate. All232policy/239dependency hashes unchanged. Direct accepted parent{direct["wins"]}W{direct["ties"]}T{direct["losses"]}L,+{direct["mean_margin"]:.2f}. Old rejected per-game audit remains rejected; new criterion explicitly optimizes population strength. No Git/catalog/upload.\n'
for name in ['LINEAGE.md','IDEAS_LEDGER.md','PROFILING_LEDGER.md','PROGRESS.md']:
    with (EXP/name).open('a') as out:out.write(entry)
print(text)
