"""Freeze a public-spending response revision and its unchanged acceptance gates."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
NAME = 'rival_wool_context_v3'
OLD = EXP / 'runs/rival_wool_context_002'


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def main():
    diagnosis = json.loads((RUN / 'PUBLIC_SPENDING_DIAGNOSIS.json').read_text())
    assert diagnosis['witness']['public_extra_spend_after_hires_and_wheat'] == 0
    policy = (OLD / 'source/policy.hpp').read_text().replace('rival_wool_context_v2', NAME)
    policy = policy.replace('    bool pending_=false,active_=false;',
        '    std::array<int,kag::N_PRODUCTS> old_market_{};\n'
        '    double old_rival_money_=0, rival_extra_spend_=0;\n'
        '    int old_rival_hires_=0;\n'
        '    bool pending_=false,active_=false;')
    policy = policy.replace('        first_=second_=farmer_=',
        '        old_market_={};old_rival_money_=rival_extra_spend_=0;old_rival_hires_=0;\n'
        '        first_=second_=farmer_=')
    policy = policy.replace('                std::copy_n(alternative.orders+6,4,deferred_.begin());',
        '                std::copy_n(o.market.inventory,kag::N_PRODUCTS,old_market_.begin());\n'
        '                old_rival_money_=o.opponent().money;old_rival_hires_=o.opponent().hires_today;\n'
        '                std::copy_n(alternative.orders+6,4,deferred_.begin());')
    policy = policy.replace('            const bool context=v2_context(first_,second_);',
        '''            bool only_wheat=true;
            for(int item=1;item<kag::N_PRODUCTS;++item) {
                int demand=item!=kag::FERTILIZER && 144%config_.center_sell_interval==0;
                if(144%config_.shop_sell_interval==0)for(int i=0;i<o.n_shops;++i)
                    if(kag::SHOP_MASK[o.shops[i]]&(1u<<item))demand+=kag::SHOP_MULT[o.shops[i]];
                only_wheat &= o.market.inventory[item]-old_market_[item]+demand==0;
            }
            int hire_cost=0;
            for(int hire=old_rival_hires_;hire<o.opponent().hires_today;++hire)
                hire_cost+=config_.hire_mult*kag::fib(hire);
            rival_extra_spend_=old_rival_money_-o.opponent().money-hire_cost-
                kag::market_price(kag::WHEAT,old_wheat_-1);
            const bool context=v2_context(first_,second_);''')
    before = 'o.opponent().n_units==7 && rival_wheat_flow_==-1 && o.market.inventory[kag::FERTILIZER]==old_fertilizer_;'
    after = 'o.opponent().n_units==7 && old_rival_hires_==0 && o.opponent().hires_today==6 &&\n                rival_wheat_flow_==-1 && only_wheat && rival_extra_spend_>0;'
    assert policy.count(before) == 1
    policy = policy.replace(before, after)
    policy = policy.replace('// for an observed early wheat purchase with the six-hire wool prefix.',
        '// for a six-hire prefix, one net wheat purchase, no other product flow,\n'
        '    // and observed spending beyond those hires and the wheat quote.')
    (RUN / 'source').mkdir(exist_ok=False)
    (RUN / 'source/policy.hpp').write_text(policy)
    package = RUN / 'proposals' / NAME
    (package / 'source').mkdir(parents=True)
    write(package / 'agent.json', {'format_version': 1, 'name': NAME, 'header': 'source/agent.hpp',
          'type': f'kag::agents::{NAME}::Agent', 'sources': ['source/agent.cpp',
          '../../../../league/top_replay_library/source/agent.cpp',
          '../../../../league/public_router/source/agent.cpp']})
    (package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/policy.hpp"\n'
        f'namespace kag::agents::{NAME}{{class Agent:public compositions::{NAME}::Policy{{public:Agent():Policy(2){{}} static kag::agent::AgentInfo info(){{return {{"{NAME}"}};}}}};}}\n')
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (package / 'README.md').write_text('''# Rival wool context v3

Preserve wool_family_context_v2. On its compatible day-6 seven-hire opening,
observe the next hour before choosing an additional complete wool continuation.
Require a Yarn shop, six rival hires, one net wheat purchase, no other net product
flow, and public spending beyond hiring and wheat. Entry, deferred purchases and
17 improved daily schedules come from the preceding delayed-family experiment.

External lineage, parity scope and limitations are in IMPORT.json. Validation
authority: results/rival_wool_context_v3_validation.json at the experiment root.
No Kaggle rating or new submission. Net flows cannot identify all gross trades.
''')
    origin = {'parent': 'wool_family_context_v2', 'previous_candidate': 'rival_wool_context_v2',
              'donor': 'public_router_v52', 'donor_provenance': 'league/public_router_v52/IMPORT.json',
              'calendar_lineage': 'runs/v52_family_001/DAY_LIBRARY.json',
              'bridge': 'runs/family_delay_001/BRIDGE_ANALYSIS.json',
              'diagnosis': 'runs/rival_wool_context_003/PUBLIC_SPENDING_DIAGNOSIS.json',
              'changes': ['Infer spending after observed hires and the post-buy wheat quote.',
                          'Require no other net product flow after public town/shop demand.',
                          'Preserve previous accepted early wool choices and delayed execution.'],
              'kaggle_rating': None, 'parity_scope': 'Upstream bridge and source courses verified; this revision needs its own operational and fresh audits.',
              'optimization_status': 'Frozen experimental source; see central validation for current status.',
              'reuse': 'User authorized public code and replay borrowing; original donor restrictions remain in donor import.',
              'limitations': ['Public residual is not an identity oracle; gross offsetting trades can be ambiguous.',
                              'One observed response selects a precompiled farm; arbitrary farm construction is incomplete.'],
              'previous_source_sha256': hashlib.sha256((OLD / 'source/policy.hpp').read_bytes()).hexdigest()}
    write(package / 'IMPORT.json', origin)
    registry = json.loads((EXP / 'configs/league.json').read_text())
    assert NAME not in registry
    registry[NAME] = str(package.relative_to(ROOT))
    write(EXP / 'configs/league.json', registry)
    validation = EXP / 'runs/rival_wool_validation_003'
    validation.mkdir(exist_ok=False)
    previous = EXP / 'runs/rival_wool_validation_002'
    spec = json.loads((previous / 'FRESH_PREREGISTERED.json').read_text())
    spec.update(created_utc=datetime.now(timezone.utc).isoformat(), candidate=NAME, seed_start=1950000,
                opponents=sorted(spec['opponents'] + ['rival_wool_context_v2']),
                scope='514 exposed diagnostic profiles verify the failed John sale and public spending residual. All1930000 outcomes are diagnosis. This new rule is frozen before1950000.',
                extra_requirement='Same targeted response gates as v2. All28 individual utility and mean contrasts nonnegative; positive current-group confidence lower bound; historical noninferiority; every paired parent-control margin nonnegative.')
    paths = [RUN / 'source/policy.hpp', *sorted(p for p in package.rglob('*') if p.is_file())]
    spec['candidate_files_sha256'] = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    write(validation / 'FRESH_PREREGISTERED.json', spec)
    for name in ['run_fresh.py', 'analyze_fresh.py', 'run_checks.py', 'run_native.py']:
        code = (previous / name).read_text().replace('rival_wool_context_v2', NAME)
        code = code.replace('19301907', '19501907').replace('1934000', '1954000')
        if name == 'run_checks.py':
            code = code.replace('1933000', '1913000')
            code = code.replace('net wheat/fertilizer flow', 'net product flows and public spending after hires and wheat')
        if name == 'analyze_fresh.py':
            code = code.replace('and no fertilizer inventory change, choose', 'and no other product flow plus positive spending beyond hires/wheat, choose')
        (validation / name).write_text(code)
    print('Prepared', NAME, 'against', len(spec['opponents']), 'opponents; seed1950000 unused before registration.')


if __name__ == '__main__':
    main()
