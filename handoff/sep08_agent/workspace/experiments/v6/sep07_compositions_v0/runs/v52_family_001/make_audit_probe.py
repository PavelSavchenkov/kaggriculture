from pathlib import Path

RUN = Path(__file__).resolve().parent
s = (RUN / 'probe.cpp').read_text()
s = s.replace('#include <iomanip>', '#include "source/improved.hpp"\n#include <iomanip>')
s = s.replace('if(o.hour==0 || o.step==1)', 'if(o.hour==0 || o.step==1 || o.step==146)')
s = s.replace('int main(int argc,char** argv) {\n    const auto o=options(argc,argv);', 'template<class Own> int run(const Options& o) {')
s = s.replace('Trace<kag::agents::public_router_v52::Agent> own;', 'Trace<Own> own;')
s = s.replace('"v52"', '"own"')
s += '''
int main(int argc,char** argv){
    const auto o=options(argc,argv);
    if(o.a=="source")return run<kag::agents::public_router_v52::Agent>(o);
    if(o.a=="transfer")return run<compositions::v52_family::Transfer>(o);
    if(o.a=="optimized")return run<compositions::v52_family::OptimizedTransfer>(o);
    return 2;
}
'''
(RUN / 'audit_probe.cpp').write_text(s)
b = (RUN / 'build_probe.py').read_text().replace("'probe.cpp'", "'audit_probe.cpp'").replace("output / 'probe'", "output / 'audit_probe'").replace("output / 'build.json'", "output / 'audit_build.json'")
(RUN / 'build_audit_probe.py').write_text(b)
print('Prepared donor/transfer/optimized instrumentation on the same opponent.')
