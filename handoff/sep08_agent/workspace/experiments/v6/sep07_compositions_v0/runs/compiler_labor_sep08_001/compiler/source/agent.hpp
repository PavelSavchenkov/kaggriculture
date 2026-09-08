#pragma once
#include <array>
#include <vector>
#include <utility>
#include "agents/common/api/agent_api.hpp"
#include "../../../../include/composition.hpp"

namespace compositions::compiler_labor_sep08 {
using Intent=Life;
std::span<const Life> recorded_program(int program);
Support recorded_support(int program);

class AgentCore {
public:
    AgentCore(int program=0,bool source_layout=true,bool source_support=true,bool source_service=false,int reserve_days=0,bool flexible_hiring=false,int labor_mode=0)
        : program_id_(program),reserve_days_(reserve_days),source_layout_(source_layout),source_support_(source_support),source_service_(source_service),flexible_hiring_(flexible_hiring),labor_mode_(labor_mode) {}
    AgentCore(std::vector<Intent> proposal,Support support,bool source_layout=false,bool source_support=false,bool source_service=false,int reserve_days=1,bool flexible_hiring=true,int labor_mode=0)
        : program_id_(-1),reserve_days_(reserve_days),source_layout_(source_layout),source_support_(source_support),source_service_(source_service),flexible_hiring_(flexible_hiring),labor_mode_(labor_mode),custom_(true),proposal_(std::move(proposal)),support_(support) {}
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,const kag::agent::DecisionBudget&,kag::Action& action);
private:
    int program_id_=0,reserve_days_=0;
    bool source_layout_=true,source_support_=true,source_service_=false,flexible_hiring_=false;
    int labor_mode_=0;
    std::array<int,30> estimated_hands_{};
    bool custom_=false;
    std::vector<Intent> proposal_;
    Support support_{};
    kag::agent::AgentConfig config_{};
    std::vector<Intent> intents_;
    std::array<int,30> hands_{},quadrants_{};
};

template<int Program=0,bool SourceLayout=true,bool SourceSupport=true,bool SourceService=false,int ReserveDays=0,bool FlexibleHiring=false>
class Agent:public AgentCore {
public:
    Agent():AgentCore(Program,SourceLayout,SourceSupport,SourceService,ReserveDays,FlexibleHiring) {}
    static kag::agent::AgentInfo info() {return {"composition_greedy"};}
};
}
