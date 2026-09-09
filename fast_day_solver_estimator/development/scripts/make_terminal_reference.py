"""Keep the first terminal wrapper intact and derive an explicit-deadline one."""
import hashlib
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    source = EXP / "source/reference.cpp"
    target = EXP / "source/reference_terminal_deadlines.cpp"
    text = '#include "terminal_deadlines.hpp"\n' + source.read_text()
    old = "            const auto result = day_scheduler::solve(problem, options);"
    assert text.count(old) == 1
    text = text.replace(old, "            auto requested = problem;\n            labor::offline::require_terminal_work(requested);\n            const auto result = day_scheduler::solve(requested, options);")
    with target.open("x") as stream: stream.write(text)
    print(json.dumps({"source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "target_sha256": hashlib.sha256(target.read_bytes()).hexdigest()}))


if __name__ == "__main__":
    main()
