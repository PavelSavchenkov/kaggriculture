"""Give animal purchases a measured funding hour; preserve the failed compiler."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
old = RUN/'source/compile_v2.cpp'
new = RUN/'source/compile_v3.cpp'
assert not new.exists()
source = old.read_text()
before='if(delta<0 && !add_order(markets,0,22,is_animal(i)?M_BUY_ANIMAL:M_BUY_PRODUCT,i,-delta))return 3;'
assert source.count(before)==1
source = source.replace(before,'''if(delta<0){
                bool added=false;
                if(is_animal(i)){
                    auto funding_source=source;funding_source.start=sim;
                    added=insert_funded_order(funding_source,problem,markets,season.shops,M_BUY_ANIMAL,i,-delta);
                }else added=add_order(markets,0,22,M_BUY_PRODUCT,i,-delta);
                if(!added){std::ofstream(directory/"STATUS.json")<<"{\\"status\\":\\"funding_insertion_failed\\",\\"day\\":"<<day<<"}\\n";return 3;}
            }''')
new.write_text(source)
cmake=RUN/'CMakeLists.txt'
text=cmake.read_text()
assert text.count('foreach(target estimate compile compile_v2)')==1
cmake.write_text(text.replace('foreach(target estimate compile compile_v2)','foreach(target estimate compile compile_v2 compile_v3 funding_witness)'))
(RUN/'FUNDED_COMPILER.json').write_text(json.dumps({'original_sha256':hashlib.sha256(old.read_bytes()).hexdigest(),
    'new_sha256':hashlib.sha256(new.read_bytes()).hexdigest(),
    'change':'Insert extra animal purchase at the earliest hour whose recorded-day funding replay pays the whole quantity and preserves other accepted orders.',
    'origin':'Persistent experiment include/day_compile_helpers.hpp insert_funded_order, reused without changes.',
    'limitation':'The funding probe keeps recorded rival actions and original worker routes. The subsequent full live-opponent endpoint certificate remains mandatory.'},indent=2)+'\n')
