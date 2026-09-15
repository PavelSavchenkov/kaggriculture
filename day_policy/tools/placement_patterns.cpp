#include "trace/case.hpp"
#include "trace/field_effect.hpp"
#include "routes.hpp"
#include <iostream>
using namespace kag;
using namespace kag::agents::day_policy_contract;
using namespace schedule_search;

int main(int argc,char** argv) {
    if(argc!=3)throw std::runtime_error("placement_patterns cohort.txt output.csv");
    std::ifstream cohort(argv[1]);std::ofstream out(argv[2]);
    out<<"game,seat,rank,day,hour,item,tile,shed_distance,reuse,nearest_animal,nearest_crop,nearest_same,near_free,free_sites\n";
    int game_id,seat,rank,games=0;std::string path;
    while(cohort>>game_id>>seat>>rank>>path) {
        auto game=load_case(path);auto states=validate_case(game);
        bool cleared[100]{};
        for(int step=0;step<719;++step) {
            const auto& sim=states[step];const auto& f=sim.st.farms[seat];
            if(step%24==0)std::fill_n(cleared,100,false);
            const auto actions=sim.sanitize_joint_actions(game.turns[step].actions[0],game.turns[step].actions[1]);
            Tile tiles[100];std::copy_n(&f.tiles[0][0],100,tiles);
            for(int u=0;u<f.n_units;++u) {
                const auto a=actions[seat].units[u];const int cell=f.pos_y[u]*10+f.pos_x[u];
                const bool establish=a.op==OP_PLANT || (a.op==OP_PLACE && is_animal(a.arg) && !tiles[cell].has_animal &&
                    tiles[cell].kind==(ANIMALS[a.arg-GOOSE].structure==ST_COOP?T_COOP:T_PASTURE));
                if(establish) {
                    int animal=20,crop=20,same=20,near=20,free=0;
                    for(int c=0;c<100;++c) {
                        const auto& t=tiles[c];const int d=distance(c,cell);
                        if(t.has_animal)animal=std::min(animal,d);
                        if(t.kind==T_PLANT)crop=std::min(crop,d);
                        if((t.has_animal || t.kind==T_PLANT) && t.what==a.arg)same=std::min(same,d);
                        if(t.kind==T_EMPTY || t.kind==T_WEED || c==cell) {++free;near=std::min(near,shed_distance(c));}
                    }
                    out<<game_id<<','<<seat<<','<<rank<<','<<step/24<<','<<step%24<<','<<int(a.arg)<<','<<cell<<','
                       <<shed_distance(cell)<<','<<cleared[cell]<<','<<animal<<','<<crop<<','<<same<<','<<near<<','<<free<<'\n';
                }
                if(a.op==OP_DIG || (a.op==OP_HARVEST && tiles[cell].kind==T_PLANT && !CROPS[tiles[cell].what].ongoing))cleared[cell]=true;
                field_effect(tiles[cell],a);
            }
        }
        std::cout<<++games<<" game="<<game_id<<" seat="<<seat<<'\n'<<std::flush;
    }
}
