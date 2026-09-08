"""Freeze hashes and final measured status without promoting a policy."""
import hashlib
import json
from datetime import datetime,timezone
from pathlib import Path

RUN=Path(__file__).resolve().parent


def main():
    for package in (RUN/'proposals').iterdir():
        p=package/'IMPORT.json';record=json.loads(p.read_text());record['status']='Measured research control; not promoted or registered. See ../../README.md and FINAL_AUDIT.json.';p.write_text(json.dumps(record,indent=2)+'\n')
        (package/'README.md').write_text(f'# {package.name}\n\nComplete public sheep portfolio research control. See ../../README.md and ../../LINEAGE.json for exact provenance, validation and weak broad results. Not promoted.\n')
    hashes={}
    for p in sorted(RUN.rglob('*')):
        if not p.is_file()or p.name=='FINAL_AUDIT.json'or '__pycache__'in p.parts or 'source_snapshot'in p.parts:continue
        if p.suffix not in ['.json','.py','.cpp','.hpp','.inc','.md','.txt','.csv','.log']:continue
        hashes[str(p.relative_to(RUN))]=hashlib.sha256(p.read_bytes()).hexdigest()
    report={'frozen_utc':datetime.now(timezone.utc).isoformat(),'decision':'Keep complete courses/branch/terminal controls as faithful research data. No strong specialist retained or promoted.','core_source_sha256':hashes['source/agent.cpp'],'files_sha256':hashes,'checks':{'donor_exact_seat_states':2880,'source_action_parity':4314,'adaptive_games_exact_selected_control':1920,'fixed_prefix_state_cash_equal':320,'operational_records_equal_four_build_modes':16},'limitations':['Donor RNG seed withheld; transition diagnostic imposes only observed shops/new weeds','No native-shop or fresh-seed promotion audit','Terminal controls received generic discovery only','Future rival crop/replant/expansion and unknown private stock remain unmodeled']}
    (RUN/'FINAL_AUDIT.json').write_text(json.dumps(report,indent=2)+'\n');print('Frozen',len(hashes),'files',report['core_source_sha256'])


if __name__=='__main__':main()
