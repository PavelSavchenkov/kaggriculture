"""Statically unpack public source payloads without executing embedded modules."""
import ast
import base64
import hashlib
import json
import zlib
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]
ROOT=EXP.parents[2]
SOURCE=ROOT/"external/kaggriculture/agents/champion-v58/v58_agent.py"
OUT=EXP/"research/kaito_v58"


def main():
    OUT.mkdir(exist_ok=True)
    queue=[("upstream",SOURCE.read_text())]
    seen=set()
    manifest=[]
    while queue:
        name,source=queue.pop(0)
        digest=hashlib.sha256(source.encode()).hexdigest()
        if digest in seen:continue
        seen.add(digest)
        tree=ast.parse(source)
        (OUT/f"{name}.py").write_text(source)
        manifest.append({"name":name,"sha256":digest,"bytes":len(source.encode())})
        for node in tree.body:
            if not isinstance(node,ast.Assign) or not isinstance(node.targets[0],ast.Name):continue
            key=node.targets[0].id
            decoders=[call for call in ast.walk(node.value) if isinstance(call,ast.Call)
                and isinstance(call.func,ast.Attribute) and call.func.attr in {"b85decode","b64decode"}]
            if len(decoders)!=1:continue
            decoder=decoders[0]
            if not isinstance(decoder.args[0],ast.Constant):continue
            encoded=decoder.args[0].value
            raw=(base64.b85decode if decoder.func.attr=="b85decode" else base64.b64decode)(encoded)
            if any(isinstance(call,ast.Call) and isinstance(call.func,ast.Attribute) and call.func.attr=="decompress" for call in ast.walk(node.value)):
                raw=zlib.decompress(raw)
            text=raw.decode()
            if "SOURCE" in key:
                queue.append((key,text))
            else:
                (OUT/f"{key}.json").write_text(text)
                manifest.append({"name":key,"sha256":hashlib.sha256(raw).hexdigest(),"bytes":len(raw),"parent":name})
                if text.lstrip().startswith("{"):
                    bundle=json.loads(text)
                    for module,body in bundle.items():
                        if isinstance(body,str) and ("def " in body or "import " in body):
                            queue.append((module.replace("/","_").replace(".py",""),body))
    (OUT/"unpacked_manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print(json.dumps(manifest,indent=2))


if __name__=="__main__":main()
