"""Save exact care-bank witnesses and the small farm's biological output gap."""
from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
expected = json.loads((RUN/'traces/expected.json').read_text())
ideal = [sum(day[i] for day in expected['daily_output']) for i in range(9)]
records = {}
for path in sorted((RUN/'traces').glob('*.jsonl')):
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    assert len(rows) == 61 and rows[-1]['phase'] == 'terminal', path
    records[path.stem] = rows


def snapshot(mode, phase, day):
    return next(row for row in records[f'mode{mode}_1000_s0'] if row['phase']==phase and row['day']==day)


def animal(mode, phase, day, cell):
    row = snapshot(mode, phase, day)
    tile = row['tiles'][cell]
    return {'mode':mode,'phase':phase,'day':day,'cell':cell,'cash':row['cash'],
            'item':tile[1],'born':tile[3],'held':tile[4],'fed':tile[7],
            'cared':tile[8],'pending_care':tile[10]}


witnesses = []
for mode in [0,2]:
    for phase, day, cell in [('before_last_hour',5,24),('start',6,24),('start',9,24),
                              ('before_last_hour',7,34),('start',8,34),('start',10,34)]:
        witnesses.append(animal(mode, phase, day, cell))
full_sheep = animal(0,'before_last_hour',5,24)
bare_sheep = animal(2,'before_last_hour',5,24)
assert (full_sheep['pending_care'],full_sheep['cared']) == (5,0)
assert (bare_sheep['pending_care'],bare_sheep['cared']) == (4,1)
assert animal(0,'start',6,24)['pending_care'] == 0
assert animal(2,'start',6,24)['pending_care'] == 1
assert animal(0,'before_last_hour',7,34)['cared'] == 0
report = {'fullgames':len(records),'trace_parity':json.loads((RUN/'TRACE_CHECKS.json').read_text()),
    'product_order':['wheat','carrot','tomato','strawberry','melon','egg','milk','wool','fertilizer'],
    'ideal_productive_output':ideal,
    'mode_means':{str(mode):[statistics.mean(rows[-1]['produced'][i] for name,rows in records.items()
                                           if name.startswith(f'mode{mode}_')) for i in range(9)] for mode in [-1,0,2]},
    'seed1000_seat0_full_output':records['mode0_1000_s0'][-1]['produced'],
    'care_witnesses':witnesses,
    'diagnosis':'The compiler suppresses CARE with a full old bank, including the night which consumes and clears it. Today CARE should bank after that production for the next one.',
    'initial_service_caution':'Unchanged total wool when skipping initial service reflects a shifted bonus and this later cap error; it does not establish that initial care has no biological value.',
    'priority':'This small farm already realizes most biological output. Its total composition is too small compared with the league. Correct the service error, then test larger dated investment families and use exact scheduling where execution gaps appear.',
    'limits':'Requested productive output is not an attainable-profit promise. Land funding, birth dates, fertilizer and harvest schedules differ. Dense source farms still have substantial generic-compiler execution losses.'}
(RUN/'CARE_BANK_WITNESS.json').write_text(json.dumps(report,indent=2)+'\n')
(RUN/'RESULTS.md').write_text(f'''# Later-day diagnosis

All {len(records)} corrected JSONL full-game traces match both action hashes and
both cash balances from saved results. Initial bad int8 serialization is retained
separately; corrected traces and TRACE_CHECKS.json are authoritative.

Product order: wheat, carrot, tomato, strawberry, melon, egg, milk, wool, fertilizer.
Requested productive biology: {ideal}.
Full-service seed1000 seat0 realized: {report['seed1000_seat0_full_output']}.
The small farm nearly reaches its output potential. Its remaining large gap to
strong agents requires a larger/more productive composition, not only better
routes. Day5 land funding still delays wheat and is a concrete planning error.

Full-service sheep have bank5 and no CARE before the day5 night, then bank0 on
day6. Skipping day0 service leaves bank4, so the compiler does CARE on day5 and
enters day6 with bank1. This shifts wool between productions and hides the value
of initial care in the earlier comparison. Cows similarly skip CARE on day7.
Production clears old bonuses before banking today CARE; the cap test must
allow that reset when a later production can use the new bonus.

The single-condition correction is isolated in ../compiler_care_sep08_001.
No change to the accepted strong agent. Preserve full-service as a useful
default with economically justified overrides, not a universal rule.
''')
print('Validated',len(records),'traces; ideal',ideal,'full',report['seed1000_seat0_full_output'])
