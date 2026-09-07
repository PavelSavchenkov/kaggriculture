"""Create an isolated compiler version with explicit execution ablations."""
import hashlib
import json
from pathlib import Path
from shutil import copytree

EXP = Path(__file__).resolve().parents[1]


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def main():
    parent = EXP / "candidates/composition_greedy_v0"
    target = EXP / "candidates/composition_greedy_v1"
    copytree(parent, target)
    hashes = {str(p.relative_to(parent)): hashlib.sha256(p.read_bytes()).hexdigest() for p in parent.rglob("*") if p.is_file()}
    header = (target / "source/agent.hpp").read_text().replace("namespace compositions::greedy {", "namespace compositions::greedy_v1 {")
    header = replace(header, "int reserve_days=0,bool flexible_hiring=false)", "int reserve_days=0,bool flexible_hiring=false,int execution_mode=0)")
    header = replace(header, "source_service_(source_service),flexible_hiring_(flexible_hiring) {}", "source_service_(source_service),flexible_hiring_(flexible_hiring),execution_mode_(execution_mode) {}")
    header = replace(header, "int reserve_days=1,bool flexible_hiring=true)", "int reserve_days=1,bool flexible_hiring=true,int execution_mode=0)")
    header = replace(header, "flexible_hiring_(flexible_hiring),custom_(true)", "flexible_hiring_(flexible_hiring),execution_mode_(execution_mode),custom_(true)")
    header = replace(header, "    bool custom_=false;", "    int execution_mode_=0;\n    bool custom_=false;")
    header = replace(header, "bool FlexibleHiring=false>", "bool FlexibleHiring=false,int ExecutionMode=0>")
    header = replace(header, "SourceService,ReserveDays,FlexibleHiring) {}", "SourceService,ReserveDays,FlexibleHiring,ExecutionMode) {}")
    (target / "source/agent.hpp").write_text(header)
    cpp = (target / "source/agent.cpp").read_text().replace("namespace compositions::greedy {", "namespace compositions::greedy_v1 {")
    cpp = replace(cpp, "if(productive && !t.cared_today && future_bonus", "if(productive && (!(execution_mode_&2) || t.fed_today) && !t.cared_today && future_bonus")
    cpp = replace(cpp, "(service?7200:1300)", "(service && !(execution_mode_&1)?7200:1300)")
    cpp = replace(cpp, "(source_service_&&item==FERTILIZER?6500:1500)", "(source_service_ && !(execution_mode_&1) && item==FERTILIZER?6500:1500)")
    (target / "source/agent.cpp").write_text(cpp)
    manifest = {"format_version": 1, "name": "composition_greedy_v1", "header": "source/agent.hpp",
                "type": "compositions::greedy_v1::Agent<4,true,true,true,1,true,1>", "sources": ["source/agent.cpp"]}
    (target / "agent.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (target / "IMPORT.json").write_text(json.dumps({"parent": str(parent.relative_to(EXP)), "parent_hashes": hashes,
        "mode_bits": {"0": "Preserved parent behavior", "1": "Use normal fertilizer job/pickup values while preserving fertilizer-before-water dependency",
                      "2": "Create CARE job only after the animal is fed"},
        "source": "Local compiler diagnostics: source fertilizer value7200/pickup6500 dominate normal FEED1200/pickup1700; source profiles show missed service. Same recorded composition/support/source attribution retained."}, indent=2) + "\n")
    (target / "README.md").write_text("# composition_greedy_v1\n\nIsolated execution ablations over composition_greedy_v0. Mode0 preserves the parent; bit1 lowers recorded fertilizer/pickup priorities to productive-mode levels; bit2 allows CARE only after FEED. "
        "Fertilizer-before-water dependencies remain. Indexed and arbitrary dated composition interfaces are preserved. The manifest exposes program4, source layout/support/service, one-day input reserve, flexible hiring, mode1. "
        "No strength claim; mode0 parity, ablations and deployment checks pending. Exact parent source hashes and replay metadata are retained.\n")
    catalog_path = EXP / "configs/league.json"
    catalog = json.loads(catalog_path.read_text())
    catalog["composition_greedy_v1"] = str(target.relative_to(EXP.parents[2]))
    catalog_path.write_text(json.dumps(catalog, indent=2) + "\n")
    print(target)


if __name__ == "__main__":
    main()
