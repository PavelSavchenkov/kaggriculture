"""Verify immutable release identities without running training."""
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def training_hash(stage):
    digest = hashlib.sha256()
    for path in sorted((ROOT / "training" / stage / "bc").glob("*.py")):
        digest.update(path.name.encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def main():
    selection = json.loads((ROOT / "provenance" / "selection.json").read_text())
    model = ROOT / "model" / "model.bin"
    if model.stat().st_size != selection["model"]["bytes"] or sha256(model) != selection["model"]["sha256"]:
        raise ValueError("Model identity mismatch")
    if model.read_bytes()[:8] != b"BCW23004":
        raise ValueError("Unexpected native model format")
    expected_training = {
        "stage1": "a6b1adf2982b66523ab539892552ed38d98272a073cb9bd6c4d6de6a9e055235",
        "stage2": "0788fdadae7aed7976515899a1cfca39c04f9de394b42a80d8c5c5489106e78c",
    }
    for stage, expected in expected_training.items():
        if training_hash(stage) != expected:
            raise ValueError(f"{stage} training source mismatch")
    manifest = json.loads((ROOT / "provenance" / "compiler_manifest.json").read_text())
    if manifest["source_hash"] != selection["compiler"]["source_hash"]:
        raise ValueError("Compiler selection mismatch")
    for relative, expected in manifest["files"].items():
        if not relative.startswith(("source/", "worker/")):
            continue
        path = ROOT / "day_compiler" / relative
        if not path.is_file() or sha256(path) != expected:
            raise ValueError(f"Compiler file mismatch: {relative}")
    print("verified model, native format, compiler files, and both training source snapshots")


if __name__ == "__main__":
    main()
