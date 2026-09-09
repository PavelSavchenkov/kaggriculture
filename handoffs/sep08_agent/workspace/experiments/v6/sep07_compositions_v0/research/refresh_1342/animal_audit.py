"""Reuse the audited offline extractor on the new global cohort only."""
import hashlib
import importlib.util
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
source = EXP / 'scripts/analyze_animal_decision_replays.py'
spec = importlib.util.spec_from_file_location('animal_replay_audit', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.OUT = RUN / 'animal_decisions'
module.COHORTS = [RUN.name]
module.main()
(module.OUT / 'EXTRACTOR.json').write_text(json.dumps({'source': str(source.relative_to(EXP)), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'modification': 'Only output directory and selected cohort overridden; original extraction logic unchanged.'}, indent=2) + '\n')
