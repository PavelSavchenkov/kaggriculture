"""Build isolated terminal-overlay controls from the validated full courses."""
import hashlib
import json
import subprocess
from pathlib import Path
from screen import RIVALS

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
NAMES=['sheep_fixed_small_terminal','sheep_fixed_expansion_terminal','sheep_yarn2_terminal']


def main():
    for mode,name in enumerate(NAMES):
        p=RUN/'proposals'/name;(p/'source').mkdir(parents=True,exist_ok=True)
        (p/'source/agent.hpp').write_text('#pragma once\n#include "../../../source/agent.hpp"\n#include "experiments/v6/sep07_compositions_v0/include/terminal_layer.hpp"\n'+f'namespace compositions::{name}{{class Agent:public TerminalAgent<sheep_portfolio::Agent,3>{{public:Agent():TerminalAgent(sheep_portfolio::Agent({mode})){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
        (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../source/agent.cpp']},indent=2)+'\n')
        (p/'README.md').write_text(f'# {name}\n\nComplete source course plus existing terminal layer mode3. Experimental control, not promoted. See ../../LINEAGE.json.\n')
        layer=EXP/'include/terminal_layer.hpp'
        (p/'IMPORT.json').write_text(json.dumps({'lineage':'../../LINEAGE.json','terminal_layer':str(layer.relative_to(EXP)),'terminal_sha256':hashlib.sha256(layer.read_bytes()).hexdigest(),'terminal_ideas':'Existing local Dusta/King latest-departure recall and Lynn final-stock liquidation, adapted to actual shed capacity. Exact component history remains in EXP LINEAGE.md and submitted IMPORT; no private source inference.','mode':mode,'status':'Experimental terminal control'},indent=2)+'\n')
    body=(RUN/'build.py').read_text().replace('from generate import NAMES','NAMES = '+repr(NAMES)).replace("RUN / 'build' / (","RUN / 'build' / 'terminal' / (")
    body=body.replace("main_sources = [EXP / 'src/arena.cpp'] + ([] if checked or args.reference else [RUN / 'source/diagnostics.cpp'])","main_sources = [EXP / 'src/arena.cpp']")
    (RUN/'build_terminal.py').write_text(body)
    subprocess.run(['conda','run','-n','kaggriculture','python',str(RUN/'build_terminal.py')],check=True)
    commands=[]
    for name in NAMES:
        for rival in RIVALS:
            command=['conda','run','-n','kaggriculture',str(RUN/'build/terminal/generic/arena'),'--a',name,'--b',rival,'--games','32','--seed-start','1000','--seat-mode','both','--threads','4','--validate','--output',str(RUN/f'results/discovery_{name}_vs_{rival}.json')]
            subprocess.run(command,check=True);commands.append(command)
    (RUN/'TERMINAL_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')


if __name__=='__main__':main()
