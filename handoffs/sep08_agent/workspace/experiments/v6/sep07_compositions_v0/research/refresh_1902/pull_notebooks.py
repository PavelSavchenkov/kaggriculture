from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
import hashlib
import json
import subprocess

RUN=Path(__file__).resolve().parent
changes=json.loads((RUN/'notebook_changes.json').read_text())
def pull(row):
    folder=RUN/'notebooks'/row['ref']
    folder.mkdir(parents=True)
    command=['conda','run','-n','kaggriculture','kaggle','kernels','pull',row['ref'],'-p',str(folder),'-m']
    with (folder/'pull.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    notebooks=list(folder.glob('*.ipynb'))
    scripts=list(folder.glob('*.py'))
    if notebooks:
        assert len(notebooks)==1
        notebook=json.loads(notebooks[0].read_text())
        source='\n\n'.join(''.join(cell['source']) for cell in notebook['cells'] if cell['cell_type']=='code')
    else:
        assert len(scripts)==1
        source=scripts[0].read_text()
    (folder/'source.txt').write_text(source)
    record={**row,'retrieved_utc':datetime.now(timezone.utc).isoformat(),'command':command,'code_bytes':len(source.encode()),
            'code_sha256':hashlib.sha256(source.encode()).hexdigest(),'scope':'Downloaded and extracted as text only. No notebook code executed.'}
    (folder/'PULL.json').write_text(json.dumps(record,indent=2)+'\n')
    print(row['ref'],record['code_bytes'],record['code_sha256'],flush=True)
    return record
with ThreadPoolExecutor(max_workers=2) as pool:records=list(pool.map(pull,changes))
(RUN/'NOTEBOOK_PULLS.json').write_text(json.dumps(records,indent=2)+'\n')
