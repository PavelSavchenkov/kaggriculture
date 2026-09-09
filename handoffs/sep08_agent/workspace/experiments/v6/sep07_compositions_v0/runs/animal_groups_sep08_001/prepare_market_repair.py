"""Cancel early sales against missing opening stock before valuing later output."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
old = RUN / 'source/compile_v5.cpp'
new = RUN / 'source/compile_v6.cpp'
assert not new.exists()
source = old.read_text()
point = '            if(next_day_tomato_sales && i==TOMATO) {'
assert source.count(point) == 1
source = source.replace(point, '''            // Missing carried output invalidates the earliest inherited sales.
            // New same-day production may replace it economically, but cannot
            // be assumed present before a worker harvests and deposits it.
            if(i!=WHEAT && i!=FERTILIZER && !is_animal(i)){
                int missing=std::max<int64_t>(0,source.problem.start.shed[i]-actual.problem.start.shed[i]);
                for(int h=0;h<24 && missing>0;++h)for(int s=0;s<markets[h].n_orders && missing>0;++s){
                    auto& order=markets[h].orders[s];
                    if(order.op!=M_SELL || order.item!=i)continue;
                    const int remove=std::min(missing,order.n);
                    order.n-=remove;missing-=remove;delta+=remove;
                    if(!order.n)order={};markets[h].finalize();
                }
            }
''' + point)
new.write_text(source)
control = (RUN / 'source/source_control_v2.cpp').read_text()
control = control.replace('for(uint64_t seed:{1000,1001})', 'for(uint64_t seed:{1000,1001,1014,1016})')
control = control.replace('for(int day=8;day<29;++day)', 'for(int day=28;day<30;++day)')
control = control.replace('            if(!replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())std::abort();', '')
(RUN / 'source/source_control_v3.cpp').write_text(control)
cmake = RUN / 'CMakeLists.txt'
text = cmake.read_text()
assert text.count('compile_v4 compile_v5)') == 1
cmake.write_text(text.replace('compile_v4 compile_v5)', 'compile_v4 compile_v5 compile_v6 source_control_v3)'))
(RUN / 'MARKET_TIMING_COMPILER.json').write_text(json.dumps({
    'source_sha256': hashlib.sha256(old.read_bytes()).hexdigest(),
    'new_sha256': hashlib.sha256(new.read_bytes()).hexdigest(),
    'trigger': 'Cow9 seed1014 day28 starts with2carrots but inherited cumulative sales require3 by hour2. Missing carry was previously removed from latest sales only.',
    'change': 'Cancel earliest non-input sales against missing opening stock. Later new output is sold separately. Keep raw trade and full endpoint certification.',
    'validation': 'Pending. Add day28/29 exact-source physical controls for four exposed seeds.'}, indent=2) + '\n')
