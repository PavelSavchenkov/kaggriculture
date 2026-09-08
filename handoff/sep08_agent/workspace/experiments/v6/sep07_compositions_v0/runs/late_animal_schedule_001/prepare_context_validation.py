from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
OUT = EXP / 'runs/late_goose_context_001'
OUT.mkdir(exist_ok=False)
for name in ['run_fresh.py', 'analyze_fresh.py']:
    original = (RUN / name).read_text()
    code = original.replace('late_goose_optimized', 'late_goose_wheat_context')
    code = code.replace('1790000', '1810000').replace('1790512', '1810512').replace('17901907', '18101907')
    code = code.replace("RUN / 'proposals/late_goose_wheat_context'", "EXP / 'runs/late_animal_schedule_001/proposals/late_goose_wheat_context'")
    code = code.replace("(RUN / 'PACKAGE_PREFIX_PARITY.json')", "(EXP / 'runs/late_animal_schedule_001/PACKAGE_PREFIX_PARITY.json')")
    code = code.replace("(RUN / 'PACKAGE_PARITY.json')", "(EXP / 'runs/late_animal_schedule_001/PACKAGE_PARITY.json')")
    code = code.replace('Freeze first promising complete goose course; no shop threshold fitting on these seeds.',
                        'Preserve parent tomato/wheat context using its existing day12 rule; previous1790000 seeds are now diagnostic. New1810000 seeds are unused validation.')
    (OUT / name).write_text(code)
(OUT / 'SCRIPT_LINEAGE.json').write_text(json.dumps({'source': str(RUN.relative_to(EXP)),
    'source_sha256': {name: hashlib.sha256((RUN / name).read_bytes()).hexdigest() for name in ['run_fresh.py', 'analyze_fresh.py']},
    'changes': ['candidate name', 'fresh seeds1810000..1810511', 'course parity source paths', 'context preservation rationale']}, indent=2) + '\n')
print(OUT)
