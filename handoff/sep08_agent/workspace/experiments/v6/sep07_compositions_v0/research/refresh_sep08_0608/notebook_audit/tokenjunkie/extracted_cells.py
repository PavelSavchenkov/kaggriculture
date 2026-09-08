from pathlib import Path
import hashlib
import io
import tarfile
import urllib.request

COMMIT = "7c50bbfb41027f31a2d4bc9470424e815f1fcef1"
EXPECTED = "7b58fa06da778b1519b81d509d28dff3481b3bbcc7a2d656e8bdfe4a22540524"
URL = f"https://raw.githubusercontent.com/woahwhattheheck/commons/{COMMIT}/revenue/kaggriculture/cloud-execution-lab/exports/titan-current.tar.gz"

with urllib.request.urlopen(URL, timeout=60) as response:
    payload = response.read()
if hashlib.sha256(payload).hexdigest() != EXPECTED:
    raise RuntimeError("Release archive does not match the pinned source.")
with tarfile.open(fileobj=io.BytesIO(payload), mode="r:gz") as archive:
    names = archive.getnames()
    if "main.py" not in names or "TITAN-CONFIG.json" not in names:
        raise RuntimeError("Required agent entrypoint or configuration is missing.")

output = Path("/kaggle/working/titan-current.tar.gz")
output.write_bytes(payload)
print(f"TITAN ready: {output.name} ({len(payload):,} bytes)")
print("SHA256:", EXPECTED)
