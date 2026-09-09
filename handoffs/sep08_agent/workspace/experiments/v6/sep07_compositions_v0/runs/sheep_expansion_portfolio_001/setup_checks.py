"""Format isolated build/parity drivers from audited local infrastructure."""
import hashlib
import json
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
old=EXP/'runs/atakan_portfolio_001'
replacements={'atakan_portfolio':'sheep_portfolio','atakan_cow':'sheep_fixed_small','atakan_sheep':'sheep_fixed_expansion','atakan_goose':'sheep_yarn2','atakan_demand':'sheep_yarn2','atakan_quotes':'sheep_yarn3','atakan_value_own':'sheep_value_own_mean','atakan_value_margin':'sheep_value_margin_s64'}
body=(old/'build.py').read_text()
start=body.index('NAMES = ');end=body.index('\n',start)
body=body[:start]+'from generate import NAMES'+body[end:]
body=body.replace("['animal_adaptive_r1_c0_b0', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']","['investment_context_guarded_001_best', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']")
for a,b in replacements.items():body=body.replace(a,b)
body=body.replace('std::array<compositions::sheep_portfolio::Estimate,3>','std::array<compositions::sheep_portfolio::Estimate,2>').replace('o.step==226','o.step==288')
(RUN/'build.py').write_text(body)
body=(old/'source/diagnostics.cpp').read_text().replace('before226','before288').replace('i<3','i<2')
(RUN/'source/diagnostics.cpp').write_text(body)
body=(EXP/'tests/refresh_1010_parity.cpp').read_text();body='#define VERIFY_KING\n#include "../source/agent.hpp"\n#include <fstream>\n#include <iostream>\nint main(int argc,char**argv){if(argc!=4)return 2;std::ifstream in(argv[1]);const int mode=std::stoi(argv[2]),branch=std::stoi(argv[3]);compositions::sheep_portfolio::Agent agent(mode);\n'+body[body.index('    int cases = 0, reset;'):]
body=body.replace('#include <fstream>', '#include "../source/audit.hpp"\n#include <fstream>').replace('argc!=4','argc!=5').replace('std::ifstream in(argv[1]);','std::ifstream in(argv[1]);std::ofstream state(argv[4]);')
body=body.replace('        kag::Action a;','        if(o.step==288){const auto physical=compositions::sheep_portfolio::physical(o);state<<"[";for(size_t i=0;i<physical.size();++i){if(i)state<<",";state<<physical[i];}state<<"]\\n";}\n        if(o.step==288 && (mode==2||mode==3)){o.n_shops=4;for(int i=0;i<4;++i)o.shops[i]=i<(branch?mode:0)?kag::SHOP_YARN_STORE:kag::SHOP_BAKERY;}\n        kag::Action a;')
(RUN/'tests/parity.cpp').write_text(body)
sources=[old/'build.py',old/'source/diagnostics.cpp',EXP/'tests/refresh_1010_parity.cpp']
(RUN/'INFRASTRUCTURE.json').write_text(json.dumps({'sources_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()for p in sources},'changes':'Namespace/count/step substitution and isolated package list; exact source parity and original build dependency-freezing workflow retained.'},indent=2)+'\n')
