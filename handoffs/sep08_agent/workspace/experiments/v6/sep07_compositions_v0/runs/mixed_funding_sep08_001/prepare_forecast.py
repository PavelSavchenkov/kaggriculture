"""Extend the existing offline forecast audit to a mixed-species course."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
EXP = RUN.parents[1]
original = RUN.parent/'animal_forecast_error_sep08_001/source/diagnostic.cpp'
source = original.read_text()
source = source.replace('    int hire_cost=0;', '    int hire_cost=0;\n    std::array<std::array<int,kag::N_ITEMS>,2> produced{};')
source = source.replace('        result.cash[p]=sim.st.farms[p].money;', '        result.cash[p]=sim.st.farms[p].money;\n        std::copy_n(sim.st.farms[p].produced,kag::N_ITEMS,result.produced[p].begin());')
old = 'const std::string& name,uint64_t seed,int day,int item,\n        const std::array<int,2>& cells'
assert old in source
source = source.replace(old,'const std::string& name,uint64_t seed,int day,\n        const std::vector<std::pair<int,int>>& edits')
source = source.replace('    for(int cell:cells){','    for(auto [cell,item]:edits){')
needle = '    row("exact_full_game",after.cash[0]-before.cash[0],after.cash[1]-before.cash[1]);'
assert needle in source
source = source.replace(needle,needle+'''
    std::ofstream biology_out(fs::path(folder).parent_path()/(name+"_BIOLOGY.csv"));
    biology_out<<"item,estimated_output_gain,realized_output_gain\\n";
    for(int p=0;p<N_PRODUCTS;++p){
        int estimated=0;for(int d=day;d<30;++d)estimated+=delta.days[d].output[p];
        biology_out<<p<<','<<estimated<<','<<after.produced[0][p]-before.produced[0][p]<<'\\n';
    }
''')
source = source[:source.index('int main(int argc')]+'''
int main(int argc,char** argv){
    if(argc!=2 || fs::exists(argv[1]))return 2;std::ofstream out(argv[1]);out<<std::setprecision(12);
    out<<"case,seed,stage,own_gain,rival_gain,margin_gain,extra_labor\\n";
    const fs::path root="experiments/v6/sep07_compositions_v0/runs/mixed_funding_sep08_001";
    check_case(out,"mixed_d9",1008,9,{{5,GOOSE},{7,SHEEP},{8,SHEEP}},root/"season_30s",95464,85488);
}
'''
target=RUN/'source/forecast.cpp';target.write_text(source)
cmake=(RUN/'CMakeLists.txt').read_text()
if 'add_executable(forecast ' not in cmake:
    target_block=cmake[cmake.index('add_executable(diagnostic '):].replace('diagnostic','forecast')
    (RUN/'CMakeLists.txt').write_text(cmake+'\n'+target_block)
commands=[['conda','run','-n','kaggriculture','cmake','-S',str(RUN),'-B',str(RUN/'build')],
    ['conda','run','-n','kaggriculture','cmake','--build',str(RUN/'build'),'--target','forecast','-j','2'],
    ['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/forecast'),str(RUN/'FORECAST.csv')]]
files=[original,target,RUN/'CMakeLists.txt',RUN/'AUDIT_PROTOCOL.json',EXP/'include/animal_investment_value.hpp']
protocol={'created_utc':datetime.now(timezone.utc).isoformat(),'commands':commands,
    'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
    'scope':'Offline mixed day9 seed1008 forecast attribution. Actual future shops/flows are diagnostic substitutions only. No policy or cached course changed.'}
(RUN/'FORECAST_PROTOCOL.json').write_text(json.dumps(protocol,indent=2)+'\n')
with (RUN/'forecast.log').open('w') as log:
    for command in commands:
        subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
assert all(hashlib.sha256((EXP/p).read_bytes()).hexdigest()==h for p,h in protocol['source_sha256'].items())
print((RUN/'FORECAST.csv').read_text())
