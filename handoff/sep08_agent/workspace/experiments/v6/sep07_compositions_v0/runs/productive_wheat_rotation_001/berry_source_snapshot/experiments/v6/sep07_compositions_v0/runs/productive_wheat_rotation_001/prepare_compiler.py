"""Make the isolated wheat compiler from the preserved root compiler source."""
from pathlib import Path

RUN = Path(__file__).resolve().parent


def main():
    text = (RUN / 'source/upstream_compile_crop_rotation.cpp').read_text()
    text = text.replace('#include "../include/crop_source_season.hpp"', '#include "season.hpp"')
    start = text.index('// Semantic donor suffix:')
    end = text.index('std::vector<TileWorkAction> source_work')
    text = text[:start] + '''// Locally inferred productive wheat template, seeded by get-some-fries.
struct Rotation {
    int cell=22, first=13, cycles=3;
    bool fertilizer=true;
};

''' + text[end:]
    start = text.index('std::vector<TileWorkAction> rotation_work')
    end = text.index('// Exact isolated tile biology')
    text = text[:start] + '''std::vector<TileWorkAction> rotation_work(const Rotation& r,int day,const Tile& tile,
                                        const std::vector<TileWorkAction>& original) {
    std::vector<TileWorkAction> work;
    auto add=[&](int op,int arg=-1){TileWorkAction a;a.op=op;a.arg=arg;work.push_back(a);};
    const int resume=r.first+4*r.cycles;
    if(day<r.first)return original;
    if(day==r.first){
        // Preserve the source's mature predecessor harvest, stopping before
        // the old replacement planting. A legal empty endpoint is verified.
        for(const auto&a:original){if(a.op==OP_PLANT)break;work.push_back(a);}
        add(OP_PLANT,WHEAT);add(OP_WATER);return work;
    }
    if(day<=resume){
        const int age=(day-r.first)%4;
        if(age==2&&r.fertilizer)add(OP_FERTILIZE);
        if(age==0||age==2||age==3)add(OP_WATER);
        if(age==0){add(OP_HARVEST,WHEAT);add(OP_PLANT,day==resume?CARROT:WHEAT);add(OP_WATER);}
    }else if(day==resume+2 || day==resume+3){
        add(OP_WATER);if(day==resume+3)add(OP_HARVEST,CARROT);
    }
    return work;
}

''' + text[end:]
    # Reject invalid plant/harvest work early in the exact biology prepass.
    text = text.replace('sim.step(action,pass);\n        if(job.op==OP_HARVEST)',
        'const auto before_tile=farm.tiles[cell/10][cell%10];\n        sim.step(action,pass);\n        if(job.op==OP_PLANT && (before_tile.kind!=T_EMPTY || farm.tiles[cell/10][cell%10].kind!=T_PLANT))std::abort();\n        if(job.op==OP_HARVEST)')
    text = text.replace('bool estimate_only=false,cancel_unused_seeds=false,next_day_tomato_sales=false;int resume=26;',
                        'bool estimate_only=false,cancel_unused_seeds=true,next_day_tomato_sales=true,fertilizer=true;int first=13,cycles=3;')
    text = text.replace('else if(flag.starts_with("--resume-day="))resume=std::stoi(std::string(flag.substr(13)));',
                        'else if(flag=="--no-fertilizer")fertilizer=false;\n        else if(flag.starts_with("--first-day="))first=std::stoi(std::string(flag.substr(12)));\n        else if(flag.starts_with("--cycles="))cycles=std::stoi(std::string(flag.substr(9)));')
    text = text.replace('if(resume!=25 && resume!=26)return 2;', 'if(first<1 || cycles<1 || first+4*cycles+3>28)return 2;')
    text = text.replace('r.cell=std::stoi(value);r.resume=resume;', 'r.cell=std::stoi(value);r.first=first;r.cycles=cycles;r.fertilizer=fertilizer;')
    text = text.replace('    const int first=rotations[0].first;\n', '')
    text = text.replace('if(next_day_tomato_sales && i==TOMATO)', 'if(next_day_tomato_sales && i<N_CROPS)')
    text = text.replace('const int carry=std::min(extra,std::max(0,config.shed_capacity-occupied));',
                        'const int carry=day<28?std::min(extra,std::max(0,config.shed_capacity-occupied)):0;')
    text = text.replace('next_day_tomato_sales && i==TOMATO?', 'next_day_tomato_sales && i<N_CROPS?')
    text = text.replace('// Keep today\'s added tomatoes through the free day-end', '// Keep today\'s added crop output through the free day-end')
    text = text.replace('compile_crop_rotation NEW_RUN SEED CELL SECONDS [--estimate-only] [--cancel-unused-seeds] [--next-day-tomato-sales]',
                        'compile_wheat NEW_RUN SEED CELLS SECONDS [--estimate-only] [--no-fertilizer] [--first-day=N] [--cycles=N]')
    # State the actual fixed-seed expenditure used by these edited courses.
    text = text.replace('Seed use is not seed expenditure: old purchases are currently retained by the compiler.',
                        'Seed-use valuation only; compiler cancels newly unnecessary source purchases where feasible.')
    (RUN / 'source/compile_wheat.cpp').write_text(text)


if __name__ == '__main__':
    main()
