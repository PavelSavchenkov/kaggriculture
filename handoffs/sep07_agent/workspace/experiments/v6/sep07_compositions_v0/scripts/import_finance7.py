"""Extract immutable public source and scaffold the isolated C++ port."""
import ast
import base64
import hashlib
import json
import shutil
import zlib
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ORIGIN = EXP / "research/notebooks/destbreso/v7-38-finance7-a-full-agent-layer-by-layer"
TARGET = EXP / "league/destbreso_finance7"


def main():
    if TARGET.exists():
        raise FileExistsError(TARGET)
    reference = EXP / "tests/reference/finance7"
    reference.mkdir(parents=True, exist_ok=True)
    source, expected = {}, {}
    notebook = json.loads(next(ORIGIN.glob("*.ipynb")).read_text())
    for cell in notebook["cells"]:
        if cell["cell_type"] != "code":
            continue
        tree = ast.parse("".join(cell["source"]))
        for node in ast.walk(tree):
            if not isinstance(node, ast.Assign) or len(node.targets) != 1:
                continue
            target = node.targets[0]
            if not isinstance(target, ast.Subscript) or not isinstance(target.value, ast.Name):
                continue
            if target.value.id not in {"SOURCES", "EXPECTED"}:
                continue
            name = ast.literal_eval(target.slice)
            if target.value.id == "EXPECTED":
                expected[name] = ast.literal_eval(node.value)
            elif isinstance(node.value, ast.Constant):
                source[name] = node.value.value
            else:
                assert name == "tape.inc"
                constants = [n.value for n in ast.walk(node.value) if isinstance(n, ast.Constant) and isinstance(n.value, str)]
                assert len(constants) == 1
                source[name] = zlib.decompress(base64.b64decode(constants[0])).decode()
    assert source and source.keys() == expected.keys()
    for name, value in source.items():
        assert Path(name).name == name
        assert hashlib.sha256(value.encode()).hexdigest() == expected[name], name
        (reference / name).write_text(value)
    shutil.copy2(ORIGIN / "extracted_agent_py.py", reference / "main.py")
    upstream = TARGET / "source/upstream"
    upstream.mkdir(parents=True)
    old = EXP / "league/teammate_shoprouter/source/upstream"
    assert expected["tape.inc"] == hashlib.sha256((old / "tape.inc").read_bytes()).hexdigest()
    for name in ["runtime_types.hpp", "tape.inc"]:
        shutil.copy2(old / name, upstream / name)
    guard = (old / "six_day_budget_guard.hpp").read_text().replace("namespace kag::native {", "namespace compositions::finance7_guard {\nusing namespace kag;")
    (upstream / "six_day_budget_guard.hpp").write_text(guard)
    policy = (old / "policy.hpp").read_text().replace("kag::native::", "compositions::finance7_guard::")
    (upstream / "policy.hpp").write_text(policy)
    original = ORIGIN / "extracted_agent_py.py"
    report = {"url": "https://www.kaggle.com/code/destbreso/v7-38-finance7-a-full-agent-layer-by-layer",
        "python_sha256": hashlib.sha256(original.read_bytes()).hexdigest(), "native_source_sha256": expected,
        "native_parent": "yhay81/three-day-shop-router version7, identical policy/tape hashes to teammate backbone",
        "layers": "destbreso mirror detector/route identification/next-sale append and chained hire financing",
        "changes": "Literal typed C++ layers; immutable shared native courses; per-instance episode state; active-worker normalization; no teammate RL selling head",
        "license": "Native chassis Apache-2.0. Layer file has attribution but no separate license declaration; user authorized public-source reuse.",
        "parity": "pending", "strength": "pending local tests; author's historical ratings are not verified current performance"}
    (TARGET / "IMPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    (TARGET / "NOTICE").write_text("Native production/router chassis: Yusuke Hayashi (yhay81), three-day-shop-router v7, Apache-2.0.\nMirror and financing layers: destbreso, v7.38 finance7 public notebook.\nLocal typed port: sep07_compositions_v0 experiment. Exact sources/hashes in IMPORT.json.\n")
    print("extracted and verified", len(source), "native files")


if __name__ == "__main__":
    main()
