"""Reuse the reviewed paired analyzer with explicit discovery factor parents."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
source = RUN.parent / 'animal_premium_sep08_001/analyze_fresh.py'
text = source.read_text()
text = text.replace("OUT=RUN/'fresh_2340000'", "OUT=RUN/'discovery'")
first = text.index("if 'animal_premium_m2' in agents:")
last = text.index('rows=[];', first)
text = text[:first] + '''pairs += [
    ('animal_repair_premium_m2', 'animal_premium_m2'),
    ('premium_q24_s216', 'opening_funding_q24'),
    ('premium_q24_s216', 'premium_sales_s216'),
    ('animal_repair_q24_premium_m2', 'animal_repair_premium_m2'),
    ('animal_repair_q24_premium_m2', 'premium_q24_s216'),
]
''' + text[last:]
text = text.replace('FRESH_RESULTS', 'RESULTS').replace('Fresh paired results', 'Discovery paired results')
text = text.replace('Completed fresh selection panel', 'Exposed discovery panel')
text = text.replace('"""Summarize completed fresh and native paired comparisons without promotion."""',
    '"""Summarize exposed combination comparisons without promotion."""')
(RUN / 'analyze.py').write_text(text)
(RUN / 'ANALYZER_LINEAGE.json').write_text(json.dumps({'source': str(source.relative_to(RUN.parents[1])),
    'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'changes': 'Discovery directory/title, explicit available factor parents. Metrics and seed-cluster bootstrap unchanged.'}, indent=2) + '\n')
print('Prepared discovery analyzer with five explicit factor-parent comparisons.')
