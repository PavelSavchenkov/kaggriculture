#pragma once
#include "intent.hpp"
#include "worker/source/policy.hpp"

namespace kag::day_compiler {
namespace worker = kag::agents::sep22_worker;
int planned_harvest_yield(const Tile& tile,int day,int events);
enum class CalendarChoice { FewerInputs, FewerActions, LessWorkToday };
struct Workload {
    worker::DayInput input{};
    worker::SolveOptions constraints{};
    int seed_need[N_CROPS]{}, animal_need[3]{}, wheat_need=0, fertilizer_need=0;
    int field_output[N_PRODUCTS]{}, field_wheat_used=0, field_fertilizer_used=0;
    int harvest_amount[100]{};
    int newborn_service_deferred[3]{};
    int newborn_first_yield_shortfall=0;
    Workload();
};
// Current-state fallback: remove optional first-day animal service while
// preserving every dawn-bound service request. Recompute shed/cargo wheat needs.
int defer_newborn_service(const Observation& current,const detail::BoundIntent& dawn_intent,Workload& remaining);
class WorkloadBuilder {
public:
    bool build(const Observation& dawn, const detail::BoundIntent& intent, Workload& output,
               CalendarChoice calendar=CalendarChoice::FewerInputs, bool optional_harvests=true,
               bool direct_field_inputs=false,bool optional_collections=true,bool productive_newborns=true);
private:
    Biology biology_;
};
}
