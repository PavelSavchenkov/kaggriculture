"""Format checked C++ animal-entry artifacts; makes no policy decisions."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', type=Path)
    parser.add_argument('sources', type=Path, nargs='+')
    args = parser.parse_args()
    output = args.output.resolve()
    assert output.is_relative_to(EXP / 'runs') and not output.exists()
    output.mkdir()
    code = '#pragma once\n#include "../../include/deferred_animal.hpp"\n'
    models, entries, records, source_agents, opportunities = [], [], [], set(), []
    hashes = {}
    for index, source in enumerate(args.sources):
        source = source.resolve()
        assert source.is_relative_to(EXP / 'runs')
        summary = json.loads((source / 'summary.json').read_text())
        source_agents.add(summary['source'])
        opportunities.append({k:summary[k] for k in ['purchase_step','purchase_order','cell','original'] if k in summary})
        force = int(summary.get('force_entry_service', True))
        models.append(f'{{{source.name}::flows(),{source.name}::service(),{force}}}')
        files = [source / 'model.hpp', source / 'compiled.csv', source / 'summary.json']
        code += f'#include "{os.path.relpath(files[0], output)}"\n'
        rows = list(csv.DictReader((source / 'compiled.csv').open()))
        for row in rows:
            if not all(row[k] == '1' for k in ('solved', 'endpoint_equal', 'finance_equal')):
                continue
            name = f'{source.name}_d{row["day"]}_i{row["item"]}'
            folder = source / name
            entry = folder / 'entry.hpp'
            assert entry.exists()
            code += f'#include "{os.path.relpath(entry, output)}"\n'
            entries.append(f'{{auto entry={name}::entry();entry.model={index};result.push_back(std::move(entry));}}')
            files.extend([entry, folder / 'problem.json', folder / 'schedule.txt'])
            records.append({'model': index, 'source': source.name, 'seed': summary['seed'], **row})
        assert sum(r['model'] == index for r in records) == summary['compiled_entries']
        for file in files:
            hashes[str(file.relative_to(EXP))] = hashlib.sha256(file.read_bytes()).hexdigest()
    code += f'namespace compositions::{output.name} {{\ninline std::vector<AnimalInvestmentModel> models(){{return {{{",".join(models)}}};}}\n'
    code += 'inline std::vector<AnimalEntry> entries(){std::vector<AnimalEntry> result;\n' + '\n'.join(entries) + '\nreturn result;}\n}\n'
    (output / 'bank.hpp').write_text(code)
    assert len(source_agents) == 1 and all(o == opportunities[0] for o in opportunities)
    (output / 'LINEAGE.json').write_text(json.dumps({'source': 'candidates/' + next(iter(source_agents)), 'opportunity': opportunities[0], 'components': records, 'files': hashes,
        'local_compiler': 'src/compile_animal_entries.cpp', 'valuation': 'include/animal_investment_value.hpp',
        'scope': 'Checked entry-day plans only; full-game realization, runtime branch coverage and strength gates remain pending.'}, indent=2) + '\n')
    print(output, len(entries), 'entries', len(models), 'baseline flow models')


if __name__ == '__main__':
    main()
