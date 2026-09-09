"""Regenerate the stricter source control and add the hint diagnostic targets."""
from pathlib import Path

RUN=Path(__file__).resolve().parent
out=RUN/'source/source_control_v2.cpp'
assert not out.exists()
source=(RUN/'source/source_control.cpp').read_text()
source=source.replace('for(int n:net)if(n)std::abort();a.finalize();',
    'for(int n:net)if(n)std::abort();\n                for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL)a.orders[j]={};\n                a.finalize();')
source=source.replace('const auto replay=replay_schedule(p,actions);',
    'const auto replay=replay_schedule(p,actions);\n            if(!replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())std::abort();')
out.write_text(source)
cmake=RUN/'CMakeLists.txt'
source=cmake.read_text()
before='foreach(target estimate compile compile_v2 compile_v3 funding_witness)'
assert source.count(before)==1
source=source.replace(before,'foreach(target estimate compile compile_v2 compile_v3 funding_witness source_control source_control_v2 hinted_solve)')
source+='\ntarget_include_directories(hinted_solve SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")\n'
source+='target_compile_definitions(hinted_solve PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)\n'
cmake.write_text(source)
