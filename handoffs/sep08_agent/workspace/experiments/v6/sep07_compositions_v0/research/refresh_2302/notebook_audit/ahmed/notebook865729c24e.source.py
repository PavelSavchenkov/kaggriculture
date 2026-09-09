%pip install -q kagglehub==1.0.2 kaggle-environments==1.32.7

from pathlib import Path
import ast
import base64
import copy
import hashlib
import json
import tarfile
import urllib.request
import zlib
import kagglehub

WORK = Path("/kaggle/working") if Path("/kaggle/working").is_dir() else Path.cwd()
EXPECTED_MAIN_SHA256 = "6eb728a40cc55f7e497add6ea2946ce38b0389c24531a208219d48587435838e"
print("Çıktı klasörü:", WORK)

import os
import ast
import base64
import hashlib
import json
import urllib.request
import zlib
from pathlib import Path

import kagglehub

# Save & Run All sırasında yeni notebook kaynağı bağlamayı önle.
# Dosyayı KaggleHub'ın normal HTTP indirme yöntemiyle al.
os.environ["DISABLE_KAGGLE_CACHE"] = "true"

DONOR_HANDLE = (
    "thomastschinkel/"
    "kaggriculture-95-5-win-rate-via-replay-routing/versions/2"
)
DONOR_SHA256 = (
    "8241246765098c50223d897739cb32076b71a303284d2e5e1c5f0fb495bb5a97"
)

donor_path = Path(
    kagglehub.notebook_output_download(
        DONOR_HANDLE,
        path="main.py",
    )
)

donor_bytes = donor_path.read_bytes()
actual_donor_sha = hashlib.sha256(donor_bytes).hexdigest()

assert actual_donor_sha == DONOR_SHA256, (
    f"Kaynak sürümü eşleşmiyor: {actual_donor_sha}"
)

# Kaynak Python dosyasını çalıştırmadan gömülü rota verilerini çıkar.
donor_tree = ast.parse(donor_bytes)

assignment = next(
    node for node in donor_tree.body
    if isinstance(node, ast.Assign)
    and isinstance(node.targets[0], ast.Tuple)
    and all(isinstance(item, ast.Name) for item in node.targets[0].elts)
    and [item.id for item in node.targets[0].elts] == ["SCHEDULES", "POLICY"]
)

donor_blob = next(
    node.value for node in ast.walk(assignment.value)
    if isinstance(node, ast.Constant)
    and isinstance(node.value, str)
)

SCHEDULES, _ = json.loads(
    zlib.decompress(base64.b85decode(donor_blob))
)

assert len(SCHEDULES) == 5
assert all(len(route) == 719 for route in SCHEDULES)

# Sonraki hücrede main.py içine eklenecek lisans.
with urllib.request.urlopen(
    "https://www.apache.org/licenses/LICENSE-2.0.txt",
    timeout=60,
) as response:
    license_bytes = response.read()

license_text = license_bytes.decode("utf-8").replace("\r\n", "\n")

assert hashlib.sha256(license_text.encode("utf-8")).hexdigest() == (
    "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30"
), "Lisans dosyasının içeriği eşleşmiyor."

print("Kaynak SHA-256 doğrulandı:", actual_donor_sha)
print("Hazır: 5 rota × 719 adım; Apache-2.0 lisansı doğrulandı.")

# Doğrulanmış v23 şasisinin ve router kodunun sıkıştırılmış metni.
# Sonraki hücre bunu açar ve main.py dosyasının SHA-256 değerini kontrol eder.
V23_TEMPLATE_B64 = """
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
"""

import base64
import hashlib
import zlib

# Satır sonlarını temizle ve iki aktarım hatasını düzelt.
template_b64 = "".join(V23_TEMPLATE_B64.split())
template_b64 = template_b64.replace("j1upVZZ0tyhW", "j1upVZ0tyhW")
template_b64 = template_b64.replace("xHeLNAMGEliRp", "xHeLNAMdliRp")

template_bytes = zlib.decompress(
    base64.b64decode(template_b64, validate=True)
)

# Açılan kodun doğrulanmış v23 şasisiyle aynı olduğunu kontrol et.
assert hashlib.sha256(template_bytes).hexdigest() == (
    "985d07220622767f8f90351c1dfd82c4b56c43407b3168692672e0435c26d841"
), "Şasi metni eşleşmiyor."

v23_template = template_bytes.decode("utf-8")
print("Şasi ve router kodu açıldı:", len(v23_template), "karakter")

base = copy.deepcopy(SCHEDULES[0])
payload = {"base": base, "patches": {}}

for branch in (1, 2, 3, 4):
    tape = copy.deepcopy(SCHEDULES[branch])
    tape[:144] = base[:144]
    if branch in (3, 4):
        tape[144:288] = copy.deepcopy(SCHEDULES[2][144:288])
    payload["patches"][str(branch)] = [
        [step, action] for step, action in enumerate(tape) if action != base[step]
    ]

route_blob = base64.b85encode(
    zlib.compress(json.dumps(payload, separators=(",", ":")).encode(), 9)
).decode()

notice = "# SPDX-License-Identifier: Apache-2.0\n"
notice += "# v23 modifications: public production router, audited Python chassis, and build tooling.\n"
notice += "# Credits: thomastschinkel, yhay81, tetsutani; offline simulator: destbreso/nikital7.\n"
notice += "".join("# " + line + "\n" for line in license_text.splitlines())

source = notice + v23_template.replace("__V23_ROUTE_BLOB__", repr(route_blob))
source_bytes = source.encode("utf-8")
actual_sha = hashlib.sha256(source_bytes).hexdigest()
assert actual_sha == EXPECTED_MAIN_SHA256, f"v23 kaynak kontrolü başarısız: {actual_sha}"
compile(source, "main.py", "exec")

MAIN = WORK / "main.py"
MAIN.write_bytes(source_bytes)
(WORK / "LICENSE.txt").write_text(license_text, encoding="utf-8")
(WORK / "ATTRIBUTION.md").write_text(
    "# Kaggriculture v23 attribution\n\n"
    "Apache-2.0 public sources:\n"
    "- thomastschinkel: five production schedules and public-state routing.\n"
    "  https://www.kaggle.com/code/thomastschinkel/kaggriculture-95-5-win-rate-via-replay-routing\n"
    "- yhay81: readable tape runtime, sell-lead and budget/terminal guard ideas.\n"
    "  https://www.kaggle.com/code/yhay81/fieldbook-commit-for-three-days\n"
    "- tetsutani: weed repair, room guard, sale clamp and dead-stock ideas.\n"
    "  https://www.kaggle.com/code/tetsutani/shape-the-shop-work-the-pasture-kaggriculture\n"
    "- destbreso/nikital7: offline C++ simulator; not shipped in the agent.\n"
    "  https://github.com/destbreso/kaggriculture-cppsim\n\n"
    "Local v23 changes: audited Python chassis, corrected sell-lead accounting,\n"
    "sequential BUY/SELL stock accounting, preserved empty market slots,\n"
    "per-seat public-state routing and exception fallbacks.\n"
    "The Apache-2.0 license is embedded in main.py.\n\n"
    "Local evaluation: top 106W/14L, opp 145W/5L, reactive arena 104W/4L.\n"
    "Live Kaggle rating >=2800 has not yet been verified.\n",
    encoding="utf-8",
)

print("main.py hazır:", len(source_bytes), "bayt")
print("SHA-256:", actual_sha)
print("Bu dosya yerelde doğrulanan v23 main.py ile birebir aynı.")

import contextlib
import io

with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
    from kaggle_environments import make
    from kaggle_environments.agent import get_last_callable

entry = get_last_callable(MAIN.read_text(encoding="utf-8"), path=str(MAIN))
assert entry.__name__ == "agent", f"Yanlış entrypoint: {entry.__name__}"

results = []
for label, opponent in (("self-play", str(MAIN)), ("starter", "starter")):
    for seed in (7, 1234):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            env = make("kaggriculture", configuration={"episodeSteps": 720, "seed": seed}, debug=True)
            steps = env.run([str(MAIN), opponent])
        last = steps[-1]
        statuses = [s["status"] for s in last]
        rewards = [s["reward"] for s in last]
        print(label, "seed=", seed, "status=", statuses, "rewards=", rewards)
        assert statuses == ["DONE", "DONE"], "Doğrulama başarısız; paketleme durduruldu."
        results.append({"opponent": label, "seed": seed, "status": statuses, "rewards": rewards})

ARCHIVE = WORK / "submission_competitive_v23.tar.gz"
with tarfile.open(ARCHIVE, "w:gz", format=tarfile.GNU_FORMAT) as package:
    package.add(MAIN, arcname="main.py")

with tarfile.open(ARCHIVE, "r:gz") as package:
    assert package.getnames() == ["main.py"]
    assert package.extractfile("main.py").read() == source_bytes

manifest = {
    "main_sha256": EXPECTED_MAIN_SHA256,
    "archive_sha256": hashlib.sha256(ARCHIVE.read_bytes()).hexdigest(),
    "archive_bytes": ARCHIVE.stat().st_size,
    "entrypoint": "agent",
    "validation": "PASS",
    "games": results,
}
(WORK / "v23_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print("\nVALIDATION PASS")
print("Submission dosyası:", ARCHIVE)
print("Arşiv boyutu:", ARCHIVE.stat().st_size, "bayt")

try:
    from IPython.display import FileLink, display
    display(FileLink(str(ARCHIVE)))
except ImportError:
    pass