from pathlib import Path
from datetime import datetime, timezone
import gzip
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PACKAGE = ROOT / 'agents/external/bohann_opening_v1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def summary(data):
    games = data['games']
    wins = sum(g['cash'] > g['opponent_cash'] for g in games)
    ties = sum(g['cash'] == g['opponent_cash'] for g in games)
    return {'games': len(games), 'wins': wins, 'ties': ties, 'losses': len(games)-wins-ties,
            'mean_margin': sum(g['cash']-g['opponent_cash'] for g in games)/len(games),
            'mean_cash': sum(g['cash'] for g in games)/len(games),
            'scenario': data['scenario']}


def run(name, opponent, games, seed, binary='arena', threads=4, native=False, expected=None):
    output = RUN / 'results' / f'{name}.json'
    output.parent.mkdir(exist_ok=True)
    command = ['conda', 'run', '-n', 'kaggriculture', str(RUN/'build'/binary),
               '--a', 'bohann_opening_v1', '--b', opponent, '--games', str(games),
               '--seed-start', str(seed), '--seat-mode', 'both', '--threads', str(threads),
               '--validate', '--output', str(output)]
    if native:
        command.append('--native-shops')
    with output.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    data = json.loads(output.read_text())
    assert len(data['games']) == games*2
    assert all(g['turns'] == 719 for g in data['games'])
    result = summary(data)
    result['command'] = command
    result['sha256'] = sha(output)
    if expected:
        before = json.loads(expected.read_text())['games']
        after = data['games']
        assert len(before) == len(after)
        for a,b in zip(before,after):
            assert a == b, (name,a['seed'],a['seat'],[k for k in a if a[k]!=b.get(k)])
        result['all_game_records_exact'] = len(before)
        result['expected_sha256'] = sha(expected)
    print(name, result['wins'],result['ties'],result['losses'], 'margin',round(result['mean_margin'],2), flush=True)
    return result


def main():
    results = {}
    old = EXP/'runs/bohann_opening_validation_001/fresh'
    for opponent in ['crop_mix_t2_wheat','investment_context_guarded_001_best','teammate_shoprouter','king_rc4','public_router_v5']:
        name = 'frozen_' + opponent
        results[name] = run(name,opponent,512,1750000,expected=old/f'bohann_opening_v1_vs_{opponent}.json')
    for opponent in ['crop_mix_t2_wheat','teammate_shoprouter','king_rc4','public_router_v5']:
        name = 'native_parity_' + opponent
        expected = EXP/f'runs/bohann_opening_checks_001/bohann_opening_v1_native_{opponent}.json'
        results[name] = run(name,opponent,128,1753000,native=True,expected=expected)
    results['generic64'] = run('generic64','teammate_shoprouter',32,1000)
    for binary,threads in [('pair',4),('debug',4),('arena',1)]:
        name = binary + '_serial64' if threads == 1 else binary + '64'
        results[name] = run(name,'teammate_shoprouter',32,1000,binary,threads,expected=RUN/'results/generic64.json')
    results['self32'] = run('self32','bohann_opening_v1',16,1753000)
    results['pass256'] = run('pass256','pass',128,1753000,
                            expected=EXP/'runs/bohann_opening_checks_001/bohann_opening_v1_pass256.json')
    for opponent in ['teammate_shoprouter','investment_context_guarded_001_best']:
        name='new_native4096_' + opponent
        results[name] = run(name,opponent,2048,1780000,native=True)
    for opponent in ['king_rc4','public_router_v5']:
        name='new_native1024_' + opponent
        results[name] = run(name,opponent,512,1780000,native=True)
    evidence = PACKAGE/'tests/evidence'
    evidence.mkdir(exist_ok=True)
    for name in results:
        source=RUN/'results'/f'{name}.json'
        target=evidence/(name+'.json.gz')
        target.write_bytes(gzip.compress(source.read_bytes(),mtime=0))
        results[name]['committed_evidence']=str(target.relative_to(PACKAGE))
        results[name]['compressed_sha256']=sha(target)
        results[name]['command']=[str(x).replace(str(ROOT)+'/','') for x in results[name]['command']]
        results[name]['command'][4]='<build-directory>/'+Path(results[name]['command'][4]).name
        results[name]['command'][results[name]['command'].index('--output')+1]='<output-directory>/'+name+'.json'
    report={'created_utc':datetime.now(timezone.utc).isoformat(),'agent':'bohann_opening_v1',
            'status':'PASS','results':results,
            'source_parity_scope':'All per-game fields, both complete action hashes, cash, biology, sales, discards, faults, worker counts; 5120 independent-shop and1024 native games versus the frozen experiment.',
            'operational':'Generic/pair/debug/serial64records exact; complete self32andPASS256; native audited. Policy namespaces isolated; legal observation API and reset semantics retained.',
            'binaries':{name:sha(RUN/'build'/name) for name in ['arena','pair','debug']}}
    (PACKAGE/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')
    (RUN/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
