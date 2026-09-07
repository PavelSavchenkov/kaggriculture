"""Statically decode King RC4's immutable source/data; never execute its code."""
import ast
import base64
import gzip
import hashlib
import json
import zlib
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
SOURCE = EXP / "research/refresh_0804/notebooks/yamakawanin/king-v4e-rc4"


def decode(node):
    if isinstance(node, ast.Call):
        function = node.func
        if isinstance(function, ast.Attribute) and isinstance(function.value, ast.Name):
            allowed = {("json", "loads"): json.loads, ("zlib", "decompress"): zlib.decompress,
                       ("base64", "b64decode"): base64.b64decode, ("base64", "b85decode"): base64.b85decode}
            key = (function.value.id, function.attr)
            assert key in allowed, key
            return allowed[key](*(decode(arg) for arg in node.args))
        if isinstance(function, ast.Attribute) and function.attr == "decode":
            return decode(function.value).decode(*(decode(arg) for arg in node.args))
        raise ValueError(ast.dump(node))
    return ast.literal_eval(node)


def main():
    tree = ast.parse((SOURCE / "cells.py.txt").read_text())
    literal = {node.targets[0].id: node.value.value for node in tree.body
               if isinstance(node, ast.Assign) and len(node.targets) == 1
               and isinstance(node.targets[0], ast.Name) and isinstance(node.value, ast.Constant)}
    raw = gzip.decompress(base64.b64decode(literal["RC4_SOURCE_GZIP_B64"]))
    assert hashlib.sha256(raw).hexdigest() == literal["EXPECTED_SOURCE_SHA256"]
    (SOURCE / "extracted_main.py").write_bytes(raw)
    code = raw.decode()
    records = {"extracted_main.py": hashlib.sha256(raw).hexdigest()}
    large = {"PROGRAM", "MARKET_LIB", "_AR_ROUTER_SRC", "_V3M_TAIL_SRC"}
    logic = []
    for node in ast.parse(code).body:
        name = node.targets[0].id if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name) else ""
        if name not in large:
            logic.append(ast.get_source_segment(code, node))
            continue
        value = decode(node.value)
        text = value if isinstance(value, str) else json.dumps(value, separators=(",", ":"))
        path = SOURCE / (name + (".py" if isinstance(value, str) else ".json"))
        path.write_text(text)
        records[path.name] = hashlib.sha256(text.encode()).hexdigest()
        logic.append(f"# Immutable {name} extracted to {path.name}")
        print(name, type(value).__name__, len(value), records[path.name])
    (SOURCE / "logic.py.txt").write_text("\n\n".join(logic) + "\n")
    (SOURCE / "EXTRACTION.json").write_text(json.dumps({"source_url": "https://www.kaggle.com/code/yamakawanin/king-v4e-rc4", "method": "Whitelisted static AST decoding; no policy/notebook execution", "sha256": records}, indent=2) + "\n")


if __name__ == "__main__":
    main()
