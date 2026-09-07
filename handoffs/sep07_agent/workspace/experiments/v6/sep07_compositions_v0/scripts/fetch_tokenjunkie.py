"""Read the immutable public release referenced by the new Kaggle notebook."""
import hashlib
import json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from urllib.request import urlopen

EXP = Path(__file__).resolve().parents[1]
COMMIT = "2dc9d9f955adb2f8ddbf238e6e5a334004178a9a"
BASE = f"https://raw.githubusercontent.com/woahwhattheheck/commons/{COMMIT}/revenue/kaggriculture/20260907-offline-agent/"
EXPECTED = "acf541d46ceb2002caf0a3bba834109a92b755fb36afa4bda9e3d966665550ac"


def main():
    folder = EXP / "research/refresh_0604/tokenjunkie_release"
    folder.mkdir(exist_ok=True)
    names = ("main.py", "LICENSE", "LICENSE-MIT.txt", "LICENSE-CC-BY-4.0.txt")
    def fetch(name):
        with urlopen(BASE + name, timeout=30) as response:
            data = response.read()
        return name, data
    with ThreadPoolExecutor(max_workers=4) as pool:
        payloads = dict(pool.map(fetch, names))
    assert hashlib.sha256(payloads["main.py"]).hexdigest() == EXPECTED
    hashes = {}
    for name, data in payloads.items():
        (folder / name).write_bytes(data)
        hashes[name] = hashlib.sha256(data).hexdigest()
    (folder / "IMPORT.json").write_text(json.dumps({"commit": COMMIT, "base_url": BASE, "files": hashes,
        "source": "tokenjunkielabs/tokenjunkielabs-farm-manager notebook, read-only immutable retrieval; source not executed"}, indent=2) + "\n")
    print(json.dumps(hashes, indent=2))


if __name__ == "__main__":
    main()
