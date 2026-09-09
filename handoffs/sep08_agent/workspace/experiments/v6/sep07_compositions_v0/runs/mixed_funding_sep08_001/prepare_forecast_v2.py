"""Correct omitted partial crop lives and last-day collection in the audit."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
EXP=RUN.parents[1]
old=RUN/'source/forecast.cpp'
source=old.read_text()
source=source.replace('    Biology delta;int saved=0;', '    Biology delta;int saved=0,saved_fertilizer=0;\n    const auto entry=load_problem_json(folder/"days"/std::to_string(day)/"problem.json");')
source=source.replace('Service service;service.feed&=~(1u<<29);service.care&=~(1u<<29);',
    'Service service;service.feed&=~(1u<<29);service.care&=~(1u<<29);service.collect_fertilizer&=~(1u<<29);')
before='''        for(const auto& life:season.lives)if(life.cell==cell && life.cohort.start_day>=day){
            const auto old=biology(life.cohort,life.service);saved+=old.seed_cost;'''
after='''        for(const auto& life:season.lives)if(life.cell==cell && life.cohort.end_day>day){
            if(!life.exact)std::abort();
            const auto old=biology(life.cohort,life.service);
            if(life.cohort.start_day>=day)saved+=old.seed_cost;
            else {
                // Already bought seed is sunk. Retain only the entry harvest
                // explicitly requested by the replacement work contract.
                for(const auto& work:entry.tile_work)if(work.tile==cell)
                    for(const auto& job:work.actions)if(job.op==OP_HARVEST && job.arg==life.cohort.item)
                        part.days[day].output[life.cohort.item]+=job.output_quantity;
            }'''
assert source.count(before)==1
source=source.replace(before,after)
source=source.replace('part.days[d].output[FERTILIZER]+=old.days[d].fertilizer;',
    'part.days[d].output[FERTILIZER]+=old.days[d].fertilizer;saved_fertilizer+=old.days[d].fertilizer;')
source=source.replace('(name+"_BIOLOGY.csv")','(name+"_BIOLOGY_V2.csv")')
source=source.replace('"item,estimated_output_gain,realized_output_gain\\n"',
    '"item,estimated_output_gain,realized_output_gain,released_inputs_as_output_equivalent\\n"')
before="        biology_out<<p<<','<<estimated<<','<<after.produced[0][p]-before.produced[0][p]<<'\\n';"
after="""        const int released=p==FERTILIZER?saved_fertilizer:0;
        biology_out<<p<<','<<estimated-released<<','<<after.produced[0][p]-before.produced[0][p]<<','<<released<<'\\n';"""
assert before in source
source=source.replace(before,after)
target=RUN/'source/forecast_v2.cpp';target.write_text(source)
cmake=(RUN/'CMakeLists.txt').read_text()
if 'add_executable(forecast_v2 ' not in cmake:
    block=cmake.split('\nadd_executable(forecast ')[0]
    block=block[block.index('add_executable(diagnostic '):].replace('diagnostic','forecast_v2')
    (RUN/'CMakeLists.txt').write_text(cmake+'\n'+block)
commands=[['conda','run','-n','kaggriculture','cmake','-S',str(RUN),'-B',str(RUN/'build')],
    ['conda','run','-n','kaggriculture','cmake','--build',str(RUN/'build'),'--target','forecast_v2','-j','2'],
    ['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),str(RUN/'build/forecast_v2'),str(RUN/'FORECAST_V2.csv')]]
files=[old,target,RUN/'season_30s/days/9/problem.json',EXP/'include/biology.hpp']
protocol={'created_utc':datetime.now(timezone.utc).isoformat(),'commands':commands,
    'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
    'scope':'Offline correction of forecast input accounting. Include active crops removed before their original harvest; retain explicitly planned entry salvage, treat existing seed cost as sunk, and stop fertilizer collection on day29 like the compiler. Released fertilizer is reported separately from production.'}
(RUN/'FORECAST_V2_PROTOCOL.json').write_text(json.dumps(protocol,indent=2)+'\n')
with (RUN/'forecast_v2.log').open('w') as log:
    for command in commands:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
assert all(hashlib.sha256((EXP/p).read_bytes()).hexdigest()==h for p,h in protocol['source_sha256'].items())
print((RUN/'FORECAST_V2.csv').read_text())
print((RUN/'mixed_d9_BIOLOGY_V2.csv').read_text())
