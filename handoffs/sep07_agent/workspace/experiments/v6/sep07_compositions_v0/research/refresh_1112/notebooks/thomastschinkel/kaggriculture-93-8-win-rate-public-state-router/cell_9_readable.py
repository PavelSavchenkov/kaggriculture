import hashlib
import tarfile
from pathlib import Path
tar_path = Path('submission.tar.gz')
with tarfile.open(tar_path, 'w:gz') as tar:
    tar.add('main.py', arcname='main.py')
sha256 = hashlib.sha256(tar_path.read_bytes()).hexdigest()
size_kb = tar_path.stat().st_size / 1024
print(f'Generated Archive: {tar_path.name}')
print(f'File Size: {size_kb:.1f} KB')
print(f'SHA256 Checksum: {sha256}')
with tarfile.open(tar_path, 'r:gz') as tar:
    archive_files = tar.getnames()
    print(f'Archive Contents: {archive_files}')
    assert 'main.py' in archive_files, 'main.py missing from archive'
print('Submission archive verified and ready to upload.')