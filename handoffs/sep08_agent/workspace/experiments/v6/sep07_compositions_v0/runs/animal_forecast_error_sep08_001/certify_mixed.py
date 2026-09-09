"""Certify the observed partial-buy repair before continuing the mixed course."""
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
original = RUN / 'source/mixed.cpp'
text = original.read_text()
before = 'if(extra){auto& a=plan[23];'
after = '''if(extra){
            for(int h:{4,6})for(int s=0;s<plan[h].n_orders;++s){auto& m=plan[h].orders[s];
                if(m.op==M_BUY_PRODUCT && m.item==WHEAT){if(m.n!=3)std::abort();--m.n;}}
            auto& a=plan[23];'''
assert text.count(before) == 1
text = text.replace(before, after)
before = 'if(extra && exact && tiles)save_actions(plan,out/"actions.txt");'
after = '''if(extra && exact && tiles){
            auto certified=problem;int corrected=0;
            for(auto& e:certified.market_plan)if(e.market_op==M_BUY_PRODUCT && e.item==WHEAT && (e.hour==4 || e.hour==6)){
                if(e.quantity!=3)std::abort();--e.quantity;++corrected;
            }
            if(corrected!=2)std::abort();
            certified.market_plan.push_back({23,int8_t(plan[23].n_orders-1),M_BUY_PRODUCT,WHEAT,2});
            day_scheduler::prepare_problem(certified);
            auto physical=load(folder/"raw_schedule_h2.txt");
            for(int h:{4,6})for(int s=0;s<physical[h].n_orders;++s){auto& m=physical[h].orders[s];
                if(m.op==M_BUY_PRODUCT && m.item==WHEAT){if(m.n!=3)std::abort();--m.n;}}
            physical[23].orders[physical[23].n_orders++]={M_BUY_PRODUCT,WHEAT,2};physical[23].finalize();
            const auto check=replay_schedule(certified,physical);
            if(!check.requirements_satisfied || !check.invariants_satisfied || !check.errors.empty()){
                for(const auto& error:check.errors)std::cerr<<error<<'\\n';return 3;
            }
            const auto destination=out/"days/9";fs::create_directories(destination);
            save_actions(plan,destination/"actions.txt");save_actions(physical,destination/"raw_schedule.txt");
            save_problem_json(certified,destination/"problem.json");
            std::ofstream(destination/"STATUS.json")<<"{\\\"physical_and_live_endpoint\\\":true}\\n";
        }'''
assert text.count(before) == 1
text = text.replace(before, after)
(RUN / 'source/mixed_certificate.cpp').write_text(text)
cmake = RUN / 'CMakeLists.txt'
base = cmake.read_text()
assert 'add_executable(mixed_certificate' not in base
block = base[base.index('add_executable(mixed '):]
cmake.write_text(base + '\n' + block.replace('mixed', 'mixed_certificate'))
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '-S', str(RUN), '-B', str(RUN / 'build')],
    ['conda', 'run', '-n', 'kaggriculture', 'cmake', '--build', str(RUN / 'build'), '--target', 'mixed_certificate', '-j', '2'],
    ['conda', 'run', '-n', 'kaggriculture', str(ROOT / 'day_solver/with_runtime.sh'), str(RUN / 'build/mixed_certificate'), str(RUN / 'mixed_certificate')],
]
(RUN / 'MIXED_CERTIFICATE_PROTOCOL.json').write_text(json.dumps({'commands': commands,
    'source': str(original.relative_to(EXP)), 'sha256': hashlib.sha256(original.read_bytes()).hexdigest(),
    'changes': 'Reduce partially funded early wheat orders3->2; replace missing2 with a funded hour23 order. Independently check the revised DayProblem/raw schedule and live day endpoint before exporting a reusable prefix.'}, indent=2) + '\n')
for i, command in enumerate(commands):
    with (RUN / f'certificate_command_{i}.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
print('Mixed day9 revised physical contract and live endpoint pass.')
