from pathlib import Path

RUN = Path(__file__).resolve().parent
path = RUN / 'source/estimate.cpp'
text = path.read_text()
(RUN / 'source/estimate_initial.cpp.txt').write_text(text)
start = text.index('                auto original_work=work_at(')
end = text.index('                deficit+=', start)
text = text[:start] + '                const auto original_work=work_at(source.days[d].problem,cell);\n                const int visits=source_visits[d][cell];\n' + text[end:]
needle = '    std::ofstream csv(out / "proposals.csv");'
cache = '''    std::array<std::array<int,100>,30> source_visits{};
    for(int day=0;day<30;++day) {
        auto replay=source.days[day].start;
        for(int hour=0;hour<24 && !replay.st.done;++hour) {
            const auto& farm=replay.st.farms[0];
            for(int unit=0;unit<farm.n_units;++unit) {
                const auto op=source.days[day].own[hour].units[unit].op;
                if(op==OP_PASS || (op>=OP_PLANT && op<=OP_CARE))
                    ++source_visits[day][farm.pos_y[unit]*10+farm.pos_x[unit]];
            }
            replay.step(source.days[day].own[hour],source.days[day].rival[hour]);
        }
    }
'''
text = text.replace(needle, cache + needle)
text = text.replace('service.care &= (1u<<std::max(0,last_production-1))-1;', 'service.care &= last_production<=29 ? (1u<<std::max(0,last_production-1))-1 : 0;')
text = text.replace('std::min(std::abs(x-3),std::abs(x-4))+std::min(std::abs(y-3),std::abs(y-4))', 'std::min(std::abs(x-4),std::abs(x-5))+std::min(std::abs(y-4),std::abs(y-5))')
path.write_text(text)
