"""Build an observation-only guard inspector for the frozen cow study."""
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
OLD = RUN.parent / 'animal_repair_sep08_001'
original = (OLD / 'source/guard_audit.cpp').read_text()
helpers = (RUN / 'source/diagnostics.cpp').read_text().split('int main(')[0]
source = helpers + '#include <sstream>\n' + original[original.index('struct Inspector {'):]
start = source.index('    animal_repair::Diagnostics diagnostics(){')
end = source.index('    static int distance', start)
source = source[:start] + '    Diagnostics diagnostics(){return diagnostic(name,policy);}\n' + source[end:]
line = 'const auto& family=animal_groups_policy::library()[d.family];'
assert source.count(line) == 1
source = source.replace(line, 'const auto& family=(name=="animal_repair_q24_premium_m2"?animal_groups_policy::library():cow_service_retained::library())[d.family];')
target = RUN / 'source/guard_audit.cpp'
target.write_text(source)
build = json.loads((RUN / 'diagnostics_build/build.json').read_text())
command = build['command']
old_source = str(RUN / 'source/diagnostics.cpp')
assert old_source in command
command[command.index(old_source)] = str(target)
command[command.index('-o')+1] = str(RUN / 'guard_audit')
with (RUN / 'guard_build.log').open('w') as log:
    subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
script = (OLD / 'audit_guards.py').read_text().replace("RUN / 'broad_2360000'", "RUN / 'discovery_2390000'")
script = script.replace("    if job['a'] == 'empty_sale_slots_m2':\n        continue\n", '')
(RUN / 'audit_guards.py').write_text(script)
(RUN / 'GUARD_AUDIT_BUILD.json').write_text(json.dumps({
    'command':command, 'source_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
    'binary_sha256':hashlib.sha256((RUN/'guard_audit').read_bytes()).hexdigest(),
    'lineage':'Original observation-only animal_repair guard inspector; select the matching immutable cow-service library. Agent behavior unchanged.'
}, indent=2)+'\n')
