"""Step 67 of dc11_local_additions.py only (usable on any tree: package trees, trees with hand-edited steps)."""
from pathlib import Path
R = Path(__file__).resolve().parents[1]
# Step 67: read the whole <model>.dc11 options line (Imitation, Sep 30: the bridge and the tools read it into char text[256], so an
# options line over 255 characters is cut mid-key and the dc11 option parser aborts at the first dawn - a crashing Kaggle agent; the live
# package's line is 205 bytes, the package line + landfirst=1 217, + regime keys 274). Bridge (source/lb_bridge.cpp) and tool readers
# (tools/full_games.cpp, tools/replay_games.cpp, tools_dc11/flip_dc11.cpp, tools_dc11/teacher_day.cpp) now read the first line in chunks.
TOOL_OLD = """    char text[256] = {0};
    if (std::FILE* file = std::fopen((model + ".dc11").c_str(), "r")) {
        if (!std::fgets(text, sizeof text, file)) text[0] = 0;
        std::fclose(file);
    }
    std::string s = text;
"""
TOOL_NEW = """    std::string s;
    if (std::FILE* file = std::fopen((model + ".dc11").c_str(), "r")) {
        for (char chunk[512]; std::fgets(chunk, sizeof chunk, file);) {  // step 67: the whole first line (no 255-character limit)
            s += chunk;
            if (s.back() == '\\n') break;
        }
        std::fclose(file);
    }
"""
BRIDGE_OLD = """            char text[256] = {0};
            if (std::fgets(text, sizeof text, file) && text[0] != 0) c->agent.options_text = text;
"""
BRIDGE_NEW = """            std::string text;  // step 67: the whole first line (a 256-byte buffer cut longer option lines mid-key -> parser abort)
            for (char chunk[512]; std::fgets(chunk, sizeof chunk, file);) {
                text += chunk;
                if (text.back() == '\\n') break;
            }
            if (!text.empty()) c->agent.options_text = text;
"""
for rel, old, new in (("source/lb_bridge.cpp", BRIDGE_OLD, BRIDGE_NEW), ("tools/full_games.cpp", TOOL_OLD, TOOL_NEW),
                      ("tools/replay_games.cpp", TOOL_OLD, TOOL_NEW), ("tools_dc11/flip_dc11.cpp", TOOL_OLD, TOOL_NEW),
                      ("tools_dc11/teacher_day.cpp", TOOL_OLD, TOOL_NEW)):
    f = R / rel
    if not f.exists():
        continue
    t = f.read_text()
    if "step 67" in t or old not in t:
        continue
    f.write_text(t.replace(old, new))
    print(f"applied: whole .dc11 line ({rel}, {t.count(old)}x)")
