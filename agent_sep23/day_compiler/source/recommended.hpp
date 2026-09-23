#pragma once
#include "reactive.hpp"

namespace kag::day_compiler {
inline CompileOptions recommended_compile_options() {
    CompileOptions options;
    options.refine_ordinary_sales=true;
    options.refine_ordinary_wheat=true;
    return options;
}
inline RepairOptions recommended_repair_options() {
    RepairOptions options;
    options.monitor_funding=true;
    return options;
}
}
