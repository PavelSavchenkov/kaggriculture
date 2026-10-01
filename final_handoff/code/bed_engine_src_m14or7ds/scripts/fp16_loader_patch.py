"""Lets Model::load (agent/bc_opus/source/agent.cpp) read fp16 network files written by scripts/fp16_model.py: a leading int32
magic 0x36314648 ("HF16") marks 2-byte weights; files without it load as before (fp32). Idempotent.
usage: fp16_loader_patch.py <agent.cpp> [...]"""
import sys
from pathlib import Path

HELPERS = '''// fp16 network files (scripts/fp16_model.py): a leading int32 magic, then the fp32 layout with 2-byte weights.
constexpr int32_t kHalfMagic = 0x36314648;

float half_to_float(uint16_t h) {
    const int exponent = (h >> 10) & 0x1f, mantissa = h & 0x3ff;
    const float magnitude = exponent == 0    ? std::ldexp(float(mantissa), -24)
                            : exponent == 31 ? (mantissa ? NAN : INFINITY)
                                             : std::ldexp(float(mantissa | 0x400), exponent - 25);
    return h & 0x8000 ? -magnitude : magnitude;
}

bool read_floats(std::FILE* f, float* out, size_t n, bool half) {
    if (!half) return std::fread(out, 4, n, f) == n;
    std::vector<uint16_t> buffer(n);
    if (std::fread(buffer.data(), 2, n, f) != n) return false;
    for (size_t i = 0; i < n; ++i) out[i] = half_to_float(buffer[i]);
    return true;
}

bool Model::load(const std::string& path) {'''

for path in sys.argv[1:]:
    p = Path(path)
    s = p.read_text()
    if "kHalfMagic" in s:
        print(f"{path}: already patched")
        continue
    edits = [
        ("bool Model::load(const std::string& path) {", HELPERS),
        ("    bool ok = std::fread(&n, 4, 1, f) == 1;\n    if (ok && n < 0) {",
         "    bool ok = std::fread(&n, 4, 1, f) == 1;\n    const bool half = ok && n == kHalfMagic;\n"
         "    if (half) ok = std::fread(&n, 4, 1, f) == 1;\n    if (ok && n < 0) {"),
        ("            ok = std::fread(c.w.data(), 4, c.w.size(), f) == c.w.size() && std::fread(c.b.data(), 4, c.b.size(), f) == c.b.size();",
         "            ok = read_floats(f, c.w.data(), c.w.size(), half) && read_floats(f, c.b.data(), c.b.size(), half);"),
        ("        ok = std::fread(layer.w.data(), 4, layer.w.size(), f) == layer.w.size() &&\n             std::fread(layer.b.data(), 4, layer.b.size(), f) == layer.b.size();",
         "        ok = read_floats(f, layer.w.data(), layer.w.size(), half) && read_floats(f, layer.b.data(), layer.b.size(), half);"),
    ]
    for old, new in edits:
        if s.count(old) != 1:
            raise SystemExit(f"{path}: anchor not found once: {old[:60]!r}")
        s = s.replace(old, new)
    if "#include <cmath>" not in s:
        s = s.replace("#include <cstdio>", "#include <cmath>\n#include <cstdio>", 1) if "#include <cstdio>" in s else "#include <cmath>\n" + s
    p.write_text(s)
    print(f"{path}: patched")
