"""Static extraction only: never execute downloaded notebook cells."""
import argparse
import ast
import base64
import hashlib
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--research-dir",type=Path,default=EXP/"research")
    args=parser.parse_args();research=args.research_dir.resolve()
    assert research.is_relative_to(EXP)
    records = []
    for notebook in sorted((research/"notebooks").rglob("*.ipynb")):
        cells = json.loads(notebook.read_text())["cells"]
        for number, cell in enumerate(cells):
            if cell["cell_type"] != "code":
                continue
            source = "".join(cell["source"])
            clean = "\n".join(line for line in source.splitlines() if not line.startswith(("%", "!")))
            if source.lstrip().startswith("%%writefile main.py"):
                code = source[source.index("\n") + 1:]
                target = notebook.parent / "extracted_main.py"
                target.write_text(code)
                records.append({"path": str(target.relative_to(EXP)), "cell": number,
                                "method": "literal writefile cell", "sha256": hashlib.sha256(code.encode()).hexdigest()})
                continue
            try:
                tree = ast.parse(clean)
            except SyntaxError:
                continue  # IPython syntax; retained unchanged in original notebook.
            for node in ast.walk(tree):
                if not isinstance(node, ast.Assign) or len(node.targets) != 1 or not isinstance(node.targets[0], ast.Name):
                    continue
                name = node.targets[0].id
                if name not in {"MAIN_B64", "AGENT_SOURCE", "agent_py"}:
                    continue
                if name == "AGENT_SOURCE" and isinstance(node.value, ast.Constant):
                    code = ast.literal_eval(node.value)
                elif name == "MAIN_B64" and isinstance(node.value, ast.Constant):
                    code = base64.b64decode(ast.literal_eval(node.value)).decode()
                elif name == "agent_py":
                    value = node.value
                    if isinstance(value, ast.Call) and isinstance(value.func, ast.Attribute) and value.func.attr == "decode":
                        value = value.func.value
                    if not isinstance(value, ast.Call) or not isinstance(value.func, ast.Attribute) or value.func.attr != "b64decode":
                        continue
                    code = base64.b64decode(ast.literal_eval(value.args[0])).decode()
                else:
                    continue
                target = notebook.parent / f"extracted_{name}.py"
                target.write_text(code)
                records.append({"path": str(target.relative_to(EXP)), "cell": number,
                                "method": f"literal AST {name}", "sha256": hashlib.sha256(code.encode()).hexdigest()})
    (research / "extracted_sources.json").write_text(json.dumps(records, indent=2) + "\n")
    print(json.dumps(records, indent=2))


if __name__ == "__main__":
    main()
