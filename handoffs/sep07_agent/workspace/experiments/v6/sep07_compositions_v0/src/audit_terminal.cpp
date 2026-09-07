#include "../include/evaluation.hpp"
#include "../include/terminal_layer.hpp"
#include "../league/justin_150/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/binghua_116/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;
template<class Opponent>
void compare(const fs::path& path,Options o) {
    o.validate=true;
    for(int mode=0;mode<4;++mode) {
        o.a="justin_terminal_"+std::to_string(mode);o.output=(path/(o.a+".json")).string();
        if(mode==0)run_batch(o,[]{return TerminalAgent<justin_150::Agent,0>{};},[]{return Opponent{};});
        if(mode==1)run_batch(o,[]{return TerminalAgent<justin_150::Agent,1>{};},[]{return Opponent{};});
        if(mode==2)run_batch(o,[]{return TerminalAgent<justin_150::Agent,2>{};},[]{return Opponent{};});
        if(mode==3)run_batch(o,[]{return TerminalAgent<justin_150::Agent,3>{};},[]{return Opponent{};});
    }
}
int main(int argc,char** argv) {
    if(argc<2)return 2;fs::path path=argv[1];if(fs::exists(path) && !fs::is_empty(path))return 2;
    fs::create_directories(path);auto o=options(argc-1,argv+1);
    if(o.b=="king_rc4")compare<king_rc4::Agent>(path,o);
    else if(o.b=="justin_150")compare<justin_150::Agent>(path,o);
    else if(o.b=="opening_router_v4")compare<opening_router_v4::Agent>(path,o);
    else if(o.b=="binghua_116")compare<binghua_116::Agent>(path,o);
    else if(o.b=="public_router")compare<public_router::Agent>(path,o);
    else if(o.b=="pass")compare<Pass>(path,o);
    else return 2;
}
