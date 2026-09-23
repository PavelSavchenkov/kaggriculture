#pragma once
#include "trajectory.hpp"
#include <fstream>
#include <memory>
#include <random>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace bc {
[[noreturn]] inline void inference_fail(const char* message) { std::fputs(message,stderr); std::abort(); }
constexpr int MAX_WIDTH=1024, INPUT_CAPACITY=11*MAX_WIDTH;
struct Layer {
    uint32_t input=0,output=0;
    std::vector<float> weight,bias;
    void run(const float* x,float* y,bool relu) const {
        for(unsigned row=0;row<output;++row) {
            float value=bias[row];const float* w=weight.data()+row*input;
            for(unsigned column=0;column<input;++column)value+=w[column]*x[column];
            y[row]=relu?std::max(0.f,value):value;
        }
    }
};
struct Scratch {
    std::array<float,INPUT_CAPACITY> input{},first{},second{};
    float context[MAX_WIDTH]{},products[9][MAX_WIDTH]{},crops[100][MAX_WIDTH]{},animals[100][MAX_WIDTH]{};
    float board[100][MAX_WIDTH]{},summary[INPUT_CAPACITY]{};
};
struct Mlp {
    std::vector<Layer> layers;
    void run(const float* input,float* output,Scratch& scratch) const {
        const float* current=input;
        for(size_t i=0;i<layers.size();++i) {
            float* next=i+1==layers.size()?output:(i%2?scratch.second.data():scratch.first.data());
            layers[i].run(current,next,i+1<layers.size());current=next;
        }
    }
};
template<class T>inline void read(std::istream& in,T& value) {
    in.read(reinterpret_cast<char*>(&value),sizeof(value));if(!in)inference_fail("truncated BC file");
}
inline void read_floats(std::istream& in,std::vector<float>& values,size_t n) {
    values.resize(n);in.read(reinterpret_cast<char*>(values.data()),n*sizeof(float));
    if(!in)inference_fail("truncated BC weights");
}
struct Prediction { dc::DayIntent intent{}; float log_probability=0; };
class Model {
public:
    uint32_t width=0,depth=0,board_enabled=0,history_enabled=0,financial_enabled=0,coordinated_enabled=0;
    uint32_t accounting_enabled=0,space_mask_enabled=0,product_plans_enabled=0;
    uint32_t probe_aux_enabled=0,probe_context_enabled=0;
    uint32_t attention_enabled=0,growth_delta_enabled=0,count_pool_enabled=0;
    uint32_t calendar_enabled=0;
    uint32_t trajectory_enabled=0;
    uint32_t service_enabled=0;
    uint32_t recurrent_enabled=0;
    Mlp global_encoder,product_encoder,crop_encoder,animal_encoder,board_encoder,fusion,crop_decoder,animal_decoder,global_decoder;
    Mlp probe_head,probe_fusion;
    Mlp attention_query,attention_gate;
    Mlp memory_input,memory_hidden;
    explicit Model(const std::string& path) {
        std::ifstream in(path,std::ios::binary);if(!in)inference_fail("cannot open BC weights\n");
        char magic[8];in.read(magic,8);if(!in)inference_fail("truncated BC weight header\n");
        const auto format=std::string(magic,8);
        if(format.substr(0,6)!="BCW230" || format[6]<'0'||format[6]>'1'||format[7]<'0'||format[7]>'9')inference_fail("unsupported BC weights");
        const int version=10*(format[6]-'0')+format[7]-'0';
        if(version<1||version>12)inference_fail("unsupported BC weights");
        read(in,width);read(in,depth);read(in,board_enabled);read(in,history_enabled);
        if(version>=2){read(in,financial_enabled);read(in,coordinated_enabled);}
        if(version>=3){read(in,accounting_enabled);read(in,space_mask_enabled);}
        if(version>=4)read(in,product_plans_enabled);
        if(version>=5){read(in,probe_aux_enabled);read(in,probe_context_enabled);}
        if(version>=6)read(in,attention_enabled);
        if(version>=7)read(in,growth_delta_enabled);
        if(version>=8)read(in,count_pool_enabled);
        if(version>=9)read(in,calendar_enabled);
        if(version>=10)read(in,trajectory_enabled);
        if(version>=11)read(in,service_enabled);
        if(version>=12)read(in,recurrent_enabled);
        uint32_t count;read(in,count);
        if(width>MAX_WIDTH||!width||depth<1||depth>8)inference_fail("unsupported model size");
        for(unsigned m=0;m<count;++m) {
            uint32_t length,n;read(in,length);if(length>64)inference_fail("invalid module name");
            std::string name(length,' ');in.read(name.data(),length);read(in,n);Mlp* module=nullptr;
            if(name=="global_encoder")module=&global_encoder;
            if(name=="product_encoder")module=&product_encoder;
            if(name=="crop_encoder")module=&crop_encoder;
            if(name=="animal_encoder")module=&animal_encoder;
            if(name=="board_encoder")module=&board_encoder;
            if(name=="fusion")module=&fusion;
            if(name=="crop_decoder")module=&crop_decoder;
            if(name=="animal_decoder")module=&animal_decoder;
            if(name=="global_decoder")module=&global_decoder;
            if(name=="probe_head")module=&probe_head;
            if(name=="probe_fusion")module=&probe_fusion;
            if(name=="attention_query")module=&attention_query;
            if(name=="attention_gate")module=&attention_gate;
            if(name=="memory_input")module=&memory_input;
            if(name=="memory_hidden")module=&memory_hidden;
            const bool memory=module==&memory_input||module==&memory_hidden;
            if(!module||!module->layers.empty()||n!=(memory?1:depth)||(memory&&!recurrent_enabled))inference_fail("unexpected BC module");
            for(unsigned k=0;k<n;++k) {
                Layer layer;read(in,layer.input);read(in,layer.output);
                if(layer.input>INPUT_CAPACITY||layer.output>INPUT_CAPACITY)inference_fail("oversized BC layer");
                read_floats(in,layer.weight,size_t(layer.input)*layer.output);read_floats(in,layer.bias,layer.output);
                module->layers.push_back(std::move(layer));
            }
        }
        char extra;if(in.read(&extra,1))inference_fail("extra BC weight bytes");
        if(global_encoder.layers.empty()||product_encoder.layers.empty()||crop_encoder.layers.empty()||animal_encoder.layers.empty()||
           fusion.layers.empty()||crop_decoder.layers.empty()||animal_decoder.layers.empty()||global_decoder.layers.empty()||
           (board_enabled&&board_encoder.layers.empty())||(probe_aux_enabled&&probe_head.layers.empty())||
           (probe_context_enabled&&(!probe_aux_enabled||probe_fusion.layers.empty()))||
           (attention_enabled&&(attention_query.layers.empty()||attention_gate.layers.empty())))inference_fail("missing BC module");
        if(recurrent_enabled&&(memory_input.layers.size()!=1||memory_hidden.layers.size()!=1||
           memory_input.layers[0].input!=49||memory_input.layers[0].output!=192||
           memory_hidden.layers[0].input!=64||memory_hidden.layers[0].output!=192))inference_fail("invalid memory layers");
        if(global_encoder.layers[0].input!=GLOBAL+16*financial_enabled+9*accounting_enabled+150*trajectory_enabled+64*recurrent_enabled)
            inference_fail("invalid global feature width");
    }
    Prediction predict(const Features& f,Scratch& s,std::mt19937_64* random=nullptr,float temperature=0) const {
        auto copy=[](float*& destination,const float* source,int n){std::copy_n(source,n,destination);destination+=n;};
        float* summary=s.summary;
        std::copy_n(f.global,GLOBAL,s.input.data());
        if(financial_enabled) {
            auto unscale=[](float value){return std::copysign(std::expm1(std::abs(value)*10.f),value);};
            const float cash=unscale(f.global[4]),price=unscale(f.products[WHEAT][0]);
            float animals=0,endangered=0,goods=0;
            for(int g=0;g<f.animal_count;++g){animals+=f.animals[g].size;endangered+=f.animals[g].size*f.animals[g].forced;}
            for(int p=CARROT;p<=WOOL;++p)goods+=(f.products[p][2]+f.products[p][3])*100.f*unscale(f.products[p][0]);
            const float wheat=(f.products[WHEAT][2]+f.products[WHEAT][3])*100.f;
            const float feed=std::max(0.f,animals-wheat)*price,survival=std::max(0.f,endangered-wheat)*price;
            const float values[]={cash/100,cash/1000,cash/10000,cash/100000,cash/(std::max(1.f,price)*100),
                animals/100,endangered/100,wheat/100,feed/1000,(cash-feed)/1000,survival/1000,(cash-survival)/1000,
                cash/300,cash/400,cash/500,goods/10000};
            for(int i=0;i<16;++i)s.input[GLOBAL+i]=std::clamp(values[i],-10.f,10.f);
        }
        if(accounting_enabled) {
            float crops=0,animals=0,mature=0,ongoing=0,species[3]{};
            for(int g=0;g<f.crop_count;++g) {
                const auto& c=f.crops[g];crops+=c.size;ongoing+=c.size*(c.key[2]+c.key[3]);
                bool due=false;for(int k=0;k<c.option_count;++k)due|=c.options[k][9]>0;
                mature+=due*c.size;
            }
            for(int g=0;g<f.animal_count;++g){animals+=f.animals[g].size;species[f.animals[g].species]+=f.animals[g].size;}
            const float area=25*f.quadrants;
            const float values[]={area,area-crops-animals,crops,animals,mature,ongoing,species[0],species[1],species[2]};
            for(int i=0;i<9;++i)s.input[GLOBAL+16*financial_enabled+i]=values[i]/100.f;
        }
        if(trajectory_enabled) {
            if(!f.trajectory_valid)inference_fail("trajectory model requires causal observed-dawn history");
            std::copy_n(f.trajectory,150,s.input.data()+GLOBAL+16*financial_enabled+9*accounting_enabled);
        }
        if(recurrent_enabled) {
            if(!f.memory_valid)inference_fail("recurrent model requires causal observed-dawn history");
            float hidden[64]{},input_gates[192],hidden_gates[192];
            auto sigmoid=[](float x){return 1.f/(1.f+std::exp(-x));};
            for(int day=0;day<=f.day;++day)if(f.memory[day][0]>0) {
                memory_input.layers[0].run(f.memory[day]+1,input_gates,false);
                memory_hidden.layers[0].run(hidden,hidden_gates,false);
                for(int k=0;k<64;++k) {
                    const float reset=sigmoid(input_gates[k]+hidden_gates[k]);
                    const float update=sigmoid(input_gates[64+k]+hidden_gates[64+k]);
                    const float candidate=std::tanh(input_gates[128+k]+reset*hidden_gates[128+k]);
                    hidden[k]=(1-update)*candidate+update*hidden[k];
                }
            }
            std::copy_n(hidden,64,s.input.data()+GLOBAL+16*financial_enabled+9*accounting_enabled+150*trajectory_enabled);
        }
        global_encoder.run(s.input.data(),summary,s);summary+=width;
        float queries[6*MAX_WIDTH],gates[6];
        if(attention_enabled) {
            attention_query.run(s.summary,queries,s);attention_gate.run(s.summary,gates,s);
            for(float& gate:gates)gate=std::tanh(gate);
        }
        for(int p=0;p<9;++p) {
            std::copy_n(f.products[p],PRODUCT,s.input.data());
            if(!history_enabled)std::fill(s.input.begin()+24,s.input.begin()+32,0.f);
            std::fill(s.input.begin()+32,s.input.begin()+41,0.f);s.input[32+p]=1;
            if(calendar_enabled) {
                std::fill(s.input.begin()+41,s.input.begin()+57,0.f);
                constexpr int first[]={2,2,8,10,10,4,8,6,0},interval[]={0,0,1,2,0,1,2,3,0},last[]={0,0,11,16,0,1000,1000,1000,0};
                for(int side=0;side<2;++side) {
                    int counts[8]{};
                    if(p<8)for(const auto& tile:f.board[side]) {
                        const int item=p<5?p:p+4;
                        if(!(p<5?tile[5]:tile[18]) || !tile[6+item])continue;
                        const int age=std::lround(tile[19]*30),held=std::lround(tile[21]*6);
                        if(age>=first[p] && held>0){++counts[0];counts[1]+=held;}
                        int due=first[p]-age;
                        if(interval[p]) {
                            const int next=age<first[p]?first[p]:first[p]+((age-first[p])/interval[p]+1)*interval[p];
                            if(next>last[p])continue;
                            due=next-age;
                        }
                        if(due>0)++counts[due<=3?due+1:due<=6?5:due<=10?6:7];
                    }
                    for(int i=0;i<8;++i)s.input[41+side*8+i]=counts[i]/100.f;
                }
            }
            product_encoder.run(s.input.data(),s.products[p],s);
        }
        pool(s.products,9,summary);
        if(attention_enabled)attend(s.products,9,queries,gates,summary);
        summary+=2*width;
        int counts[100];
        for(int g=0;g<f.crop_count;++g)crop_encoder.run(f.crops[g].key,s.crops[g],s);
        for(int g=0;g<f.crop_count;++g)counts[g]=f.crops[g].size;
        pool(s.crops,f.crop_count,summary,count_pool_enabled?counts:nullptr);
        if(attention_enabled)attend(s.crops,f.crop_count,queries+2*width,gates+2,summary);
        summary+=2*width;
        for(int g=0;g<f.animal_count;++g) {
            const auto& a=f.animals[g];std::copy_n(a.key,24,s.input.data());
            if(service_enabled) {
                const float price=std::expm1(std::abs(f.products[a.species+5][0])*10.f);
                const float wheat=std::expm1(std::abs(f.products[WHEAT][0])*10.f);
                const float held=std::round(a.key[5]*6),bank=std::round(a.key[7]*6);
                const int next_due=std::lround(a.key[20]*10),interval=a.species+1,remaining=29-f.day;
                const float cap=a.species==0?4.f:6.f;
                const bool due=next_due==1&&remaining>=1;
                const int next_care=next_due==1?next_due+interval:next_due;
                const float productions=next_due<=remaining?(remaining-next_due)/interval+1:0;
                const bool care=next_care<=remaining&&(due?0.f:bank)<cap-1;
                const float room=std::max(0.f,cap-held),without=due*std::min(room,1.f),served=due*std::min(room,1+bank);
                const float cleared=due*(std::min(cap,1+bank)-1);
                const float values[]={price/200,wheat/50,std::min(10.f,price/std::max(1.f,wheat)),next_due/10.f,
                    next_care/10.f,productions/10,due*bank/6,without/6,served/6,(served-without)/6,cleared/6,
                    float(care),care*price/200,wheat/200,((cleared+care)*price-wheat)/200,a.size*wheat/1000};
                for(int i=0;i<16;++i)s.input[24+i]=std::clamp(values[i],-10.f,10.f);
            }
            animal_encoder.run(s.input.data(),s.animals[g],s);
        }
        for(int g=0;g<f.animal_count;++g)counts[g]=f.animals[g].size;
        pool(s.animals,f.animal_count,summary,count_pool_enabled?counts:nullptr);
        if(attention_enabled)attend(s.animals,f.animal_count,queries+4*width,gates+4,summary);
        summary+=2*width;
        if(board_enabled)for(int side=0;side<2;++side) {
            for(int cell=0;cell<100;++cell)board_encoder.run(f.board[side][cell],s.board[cell],s);
            pool(s.board,100,summary);summary+=2*width;
        }
        fusion.run(s.summary,s.context,s);
        if(probe_context_enabled) {
            float probes[80];probe_head.run(s.context,probes,s);
            std::copy_n(s.context,width,s.input.data());std::copy_n(probes,80,s.input.data()+width);
            probe_fusion.run(s.input.data(),s.context,s);
        }
        Prediction result;auto& intent=result.intent;float crop_plan[72]{},animal_plan[GROUP]{},prior_groups[77]{};
        const int crop_plan_size=OPTION+60*product_plans_enabled,prior_size=17+60*product_plans_enabled;
        float logits[101];
        auto choose=[&](int minimum,int maximum) {
            if(minimum<0||maximum>100||minimum>maximum)inference_fail("empty BC action mask");
            int selected=minimum;float highest=logits[minimum];
            for(int k=minimum+1;k<=maximum;++k)if(logits[k]>highest){highest=logits[k];selected=k;}
            double probabilities[101]{},total=0;
            const float scale=temperature>0?temperature:1.f;
            for(int k=minimum;k<=maximum;++k)total+=probabilities[k]=std::exp((logits[k]-highest)/scale);
            if(temperature>0) {
                if(!random)inference_fail("sampling requires explicit RNG");
                const double draw=std::generate_canonical<double,53>(*random)*total;double sum=0;
                selected=maximum;
                for(int k=minimum;k<=maximum;++k)if((sum+=probabilities[k])>draw){selected=k;break;}
            }
            result.log_probability+=(logits[selected]-highest)/scale-std::log(total);return selected;
        };
        for(int g=0;g<f.crop_count;++g) {
            const auto& group=f.crops[g];int remaining=group.size;float previous[OPTION]{};
            for(int option=0;option<group.option_count;++option) {
                float* input=s.input.data();copy(input,s.context,width);copy(input,s.crops[g],width);
                copy(input,group.options[option],OPTION);
                for(float value:previous)*input++=value/100.f;
                *input++=(group.size-remaining)/100.f;*input++=remaining/100.f;
                if(coordinated_enabled)copy(input,prior_groups,prior_size);
                crop_decoder.run(s.input.data(),logits,s);
                int future=0;for(int j=option+1;j<group.option_count;++j)future+=group.maximum[j];
                const int selected=choose(std::max(0,remaining-future),std::min(remaining,int(group.maximum[option])));
                intent.crops[g].counts[option]=selected;remaining-=selected;
                for(int k=0;k<OPTION;++k){previous[k]+=selected*group.options[option][k];crop_plan[k]+=selected*group.options[option][k]/100.f;}
                if(product_plans_enabled)for(int p=0;p<5;++p)for(int k=0;k<OPTION;++k)
                    crop_plan[OPTION+p*OPTION+k]+=group.key[p]*selected*group.options[option][k]/100.f;
            }
            if(remaining)inference_fail("incomplete crop partition");
            if(coordinated_enabled)for(int k=0;k<group.option_count;++k) {
                const float selected=intent.crops[g].counts[k];
                for(int j=0;j<OPTION;++j)prior_groups[j]+=selected*group.options[k][j]/100.f;
                const float output=selected*group.options[k][9]*group.options[k][5]*6.f/100.f;
                for(int p=0;p<5;++p)prior_groups[12+p]+=output*group.key[p];
                if(product_plans_enabled)for(int p=0;p<5;++p)for(int j=0;j<OPTION;++j)
                    prior_groups[17+p*OPTION+j]+=group.key[p]*selected*group.options[k][j]/100.f;
            }
        }
        int retained[3]{};
        for(int g=0;g<f.animal_count;++g) {
            const auto& group=f.animals[g];float* input=s.input.data();copy(input,s.context,width);copy(input,s.animals[g],width);copy(input,crop_plan,crop_plan_size);
            animal_decoder.run(s.input.data(),logits,s);const int serve=choose(0,f.day==29?0:group.size);
            const int escape=group.forced?group.size-serve:0;
            intent.animals[g].serve_today_count=serve;intent.animals[g].allow_escape_tonight_count=escape;
            retained[group.species]+=group.size-escape;
            for(int k=0;k<GROUP;++k)animal_plan[k]+=group.key[k]*serve/100.f;
        }
        int values[9]{};bool known[9]{};constexpr int order[]={8,5,6,7,0,1,2,3,4};
        int available=25*f.quadrants;
        if(space_mask_enabled) {
            for(int g=0;g<f.animal_count;++g)available-=f.animals[g].size;
            for(int g=0;g<f.crop_count;++g) {
                available-=f.crops[g].size;
                for(int k=0;k<f.crops[g].option_count;++k)
                    if(f.crops[g].options[k][0]>0||f.crops[g].options[k][9]>0)available+=intent.crops[g].counts[k];
            }
        }
        for(int factor:order) {
            float* input=s.input.data();copy(input,s.context,width);copy(input,crop_plan,crop_plan_size);copy(input,animal_plan,GROUP);
            for(int value:values)*input++=value/100.f;
            for(bool value:known)*input++=value;
            for(int k=0;k<9;++k)*input++=k==factor;
            global_decoder.run(s.input.data(),logits,s);
            int minimum=factor>=5&&factor<8?retained[factor-5]:0,maximum=100;
            if(factor==8)maximum=f.quadrants<4;
            if(factor<5)for(int p=0;p<factor;++p)maximum-=values[p];
            if(space_mask_enabled && factor!=8) {
                int remaining=available+25*values[8];
                for(int s=0;s<(factor>=5?factor-5:3);++s)remaining-=values[5+s]-retained[s];
                if(factor<5)for(int p=0;p<factor;++p)remaining-=values[p];
                maximum=std::min(maximum,remaining+minimum);
            }
            if(growth_delta_enabled)for(int k=100;k>=minimum;--k)logits[k]=logits[k-minimum];
            values[factor]=choose(minimum,maximum);known[factor]=true;
        }
        intent.new_wheat_count=values[WHEAT];intent.new_carrot_count=values[CARROT];intent.new_tomato_count=values[TOMATO];
        intent.new_strawberry_count=values[STRAWBERRY];intent.new_melon_count=values[MELON];
        intent.target_geese_next_dawn=values[5];intent.target_cows_next_dawn=values[6];intent.target_sheep_next_dawn=values[7];
        intent.buy_next_land_today=values[8];return result;
    }
private:
    void attend(const float (*values)[MAX_WIDTH],int count,const float* queries,const float* gates,float* output) const {
        if(!count)return;
        for(int query=0;query<2;++query) {
            float scores[100],highest=-INFINITY,total=0;
            for(int n=0;n<count;++n) {
                float value=0;for(unsigned k=0;k<width;++k)value+=queries[query*width+k]*values[n][k];
                scores[n]=value/std::sqrt(float(width));highest=std::max(highest,scores[n]);
            }
            for(int n=0;n<count;++n)total+=scores[n]=std::exp(scores[n]-highest);
            for(unsigned k=0;k<width;++k) {
                float value=0;for(int n=0;n<count;++n)value+=scores[n]/total*values[n][k];
                output[query*width+k]+=gates[query]*value;
            }
        }
    }
    void pool(const float (*values)[MAX_WIDTH],int count,float* output,const int* counts=nullptr) const {
        std::fill_n(output,2*width,0.f);if(!count)return;
        int total=count;
        if(counts) { total=0;for(int i=0;i<count;++i)total+=counts[i]; }
        for(unsigned k=0;k<width;++k) {
            float sum=0,maximum=values[0][k];
            for(int i=0;i<count;++i){sum+=values[i][k]*(counts?counts[i]:1);maximum=std::max(maximum,values[i][k]);}
            output[k]=sum/std::max(1,total);output[width+k]=maximum;
        }
    }
};
}
