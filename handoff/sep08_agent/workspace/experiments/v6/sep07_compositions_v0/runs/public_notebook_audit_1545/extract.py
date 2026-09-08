"""Extract and compare public notebook source without executing notebook code."""
import ast
import base64
import difflib
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
PREVIOUS = {
    "orders": EXP / "research/refresh_0504/notebooks/az05192000gmailcom/kaggriculture-from-orders-to-actual-trades",
    "igor": EXP / "research/refresh_1342/notebooks/flexonafft/kaggriculture-smart-farm-strategy-lab",
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def extract(directory, output):
    notebook, = directory.glob("*.ipynb")
    raw = notebook.read_bytes()
    data = json.loads(raw)
    cells = data["cells"]
    code = "\n\n".join("".join(c["source"]) for c in cells if c["cell_type"] == "code")
    markdown = "\n\n".join("".join(c["source"]) for c in cells if c["cell_type"] == "markdown")
    output.mkdir(parents=True, exist_ok=True)
    (output / "code.py.txt").write_text(code)
    (output / "notes.md").write_text(markdown)
    sources = []
    for number, cell in enumerate(cells):
        if cell["cell_type"] != "code":
            continue
        source = "".join(cell["source"])
        (output / f"cell_{number}.py.txt").write_text(source)
        clean = "\n".join(line for line in source.splitlines() if not line.startswith(("%", "!")))
        if source.lstrip().startswith("%%writefile main.py"):
            agent = source[source.index("\n") + 1:]
            target = output / "extracted_main.py"
            target.write_text(agent)
            sources.append({"cell": number, "method": "literal writefile", "path": str(target.relative_to(RUN)), "sha256": sha(agent.encode())})
        try:
            tree = ast.parse(clean)
        except SyntaxError:
            continue
        readable = ast.unparse(tree)
        (output / f"cell_{number}_readable.py").write_text(readable + "\n")
        for node in ast.walk(tree):
            if not isinstance(node, ast.Assign) or len(node.targets) != 1 or not isinstance(node.targets[0], ast.Name):
                continue
            name = node.targets[0].id
            if name not in {"MAIN_B64", "AGENT_SOURCE", "agent_py"}:
                continue
            if name == "AGENT_SOURCE" and isinstance(node.value, ast.Constant):
                agent = ast.literal_eval(node.value)
            elif name == "MAIN_B64" and isinstance(node.value, ast.Constant):
                agent = base64.b64decode(ast.literal_eval(node.value)).decode()
            elif name == "agent_py":
                value = node.value
                if isinstance(value, ast.Call) and isinstance(value.func, ast.Attribute) and value.func.attr == "decode":
                    value = value.func.value
                if not isinstance(value, ast.Call) or not isinstance(value.func, ast.Attribute) or value.func.attr != "b64decode":
                    continue
                agent = base64.b64decode(ast.literal_eval(value.args[0])).decode()
            else:
                continue
            target = output / f"extracted_{name}.py"
            target.write_text(agent)
            sources.append({"cell": number, "method": f"literal AST {name}", "path": str(target.relative_to(RUN)), "sha256": sha(agent.encode())})
    return {"path": str(notebook.relative_to(EXP)), "sha256": sha(raw), "bytes": len(raw), "cells": len(cells),
            "code_sha256": sha(code.encode()), "markdown_sha256": sha(markdown.encode()),
            "sources": sources, "metadata": data.get("metadata", {})}, code, markdown


def main():
    results = {"timestamp_utc": datetime.now(timezone.utc).isoformat(), "method": "JSON, AST literals, base64 decoding; no cells executed", "notebooks": {}}
    for name, previous in PREVIOUS.items():
        current, code, notes = extract(RUN / name, RUN / name / "static")
        older, old_code, old_notes = extract(previous, RUN / name / "previous_static")
        for kind, before, after in [("code", old_code, code), ("notes", old_notes, notes)]:
            diff = "".join(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True), fromfile="previous", tofile="current"))
            (RUN / name / f"{kind}.diff").write_text(diff)
        results["notebooks"][name] = {"current": current, "previous": older, "code_equal": code == old_code, "notes_equal": notes == old_notes}
    (RUN / "EXTRACTION.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
