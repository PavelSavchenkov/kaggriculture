from pathlib import Path
from datetime import datetime, timezone
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
source = (RUN / 'search_league.cpp').read_text()
source = source.replace('argc != 3', 'argc != 5')
source = source.replace('options.games=64;', 'options.games=std::stoi(argv[3]); options.native_shops=std::stoi(argv[4]);')
source = source.replace('for (uint64_t seed=1000;seed<1064;++seed)', 'const uint64_t first=options.native_shops?1773000:1770000;\n    for (uint64_t seed=first;seed<first+options.games;++seed)')
source = source.replace('for (size_t i=0;i<choices.size();++i) for (size_t j=i;j<choices.size();++j)', 'for (size_t i=9;i==9;++i) for (size_t j=0;j<choices.size();++j)')
(RUN / 'search_fresh_league.cpp').write_text(source)
spec = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': 'q32_b13_m2',
        'reason': 'Verify the fixed candidate against all 19 newly added opening choices. Discovery has no losing matchup. Earlier counterexample was an invalid across-opponent cash comparison.',
        'independent_seeds': [1770000, 1770511], 'native_seeds': [1773000, 1773127],
        'gates': ['No other opening has greater than 52 percent utility against candidate on either fresh panel.',
                  'Candidate remains above 50 percent direct utility and positive mean margin against both Bohann and q81 hire control.',
                  'Report every matchup and any tail or draw-heavy result. Keep prior broad gates and their tradeoffs unchanged.']}
(RUN / 'FRESH_LEAGUE_PREREGISTERED.json').write_text(json.dumps(spec, indent=2) + '\n')
build = RUN / 'fresh_league_build'
build.mkdir()
command = json.loads((RUN / 'league_build/build.json').read_text())['command']
command = [str(RUN / 'search_fresh_league.cpp') if s == str(RUN / 'search_league.cpp') else str(build / 'arena') if s == str(RUN / 'league_build/arena') else s for s in command]
(build / 'build.json').write_text(json.dumps({'command': command}, indent=2) + '\n')
with (build / 'build.log').open('w') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
for native, count in [(0, 512), (1, 128)]:
    name = 'league_native' if native else 'league_fresh'
    command = ['conda', 'run', '-n', 'kaggriculture', str(build / 'arena'), str(RUN / name), str(RUN / 'discovery/choices.csv'), str(count), str(native)]
    with (RUN / (name + '.log')).open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(name, 'complete', flush=True)
