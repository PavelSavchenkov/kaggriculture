"""Verify original fixed-course actions and forced declared branch fixtures."""
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]


def main():
    body = (EXP / 'tests/refresh_1010_parity.cpp').read_text()
    body = '#define VERIFY_KING\n#include "../source/agent.hpp"\n#include "../source/audit.hpp"\n#include <fstream>\n#include <iostream>\nint main(int argc,char**argv){if(argc!=5)return 2;std::ifstream in(argv[1]);const int branch=std::stoi(argv[2]),mode=std::stoi(argv[3]);std::ofstream state(argv[4]);compositions::atakan_portfolio::Agent agent(mode);\n' + body[body.index('    int cases = 0, reset;'):]
    marker = '        kag::Action a;'
    addition = '''        if(o.step==226){auto v=compositions::atakan_portfolio::physical(o);state<<"[";for(size_t i=0;i<v.size();++i){if(i)state<<",";state<<v[i];}state<<"]\\n";
            if(mode==3){o.n_shops=2;o.shops[0]=branch==1?kag::SHOP_YARN_STORE:branch==0?kag::SHOP_ICE_CREAM_SHOP:kag::SHOP_BAKERY;o.shops[1]=branch==0?kag::SHOP_SMOOTHIE_SHOP:kag::SHOP_PET_CAFE;}
            if(mode==4){o.market.prices[kag::WOOL]=branch==1?200:190;o.market.prices[kag::MILK]=branch==0?200:100;}
        }
'''
    body = body.replace(marker, addition + marker)
    source = RUN / 'tests/parity.cpp';source.write_text(body)
    (RUN / 'build').mkdir(exist_ok=True)
    binary = RUN / 'build/parity'
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(ROOT), str(source), str(RUN / 'source/agent.cpp'), '-o', str(binary)]
    subprocess.run(command, check=True)
    reports = []
    for branch in range(3):
        for mode in [branch, 3, 4]:
            result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(binary), str(RUN / f'tests/source_{branch}.txt'), str(branch), str(mode), str(RUN / f'tests/source_state_{branch}.json')], capture_output=True, text=True)
            print(branch, mode, result.stdout, result.stderr, flush=True)
            result.check_returncode()
            reports.append({'branch': branch, 'mode': mode, 'result': result.stdout.strip()})
    (RUN / 'SOURCE_PARITY.json').write_text(json.dumps({'command': command, 'results': reports, 'scope': 'Three719-action source courses plus each forced demand/quote route on the same exogenous source observations; routing formulas are local, not original-source claims'}, indent=2) + '\n')


if __name__ == '__main__':
    main()
