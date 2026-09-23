#pragma once
#include "forecast.hpp"
#include "schedule.hpp"
#include "seller.hpp"

namespace kag::day_compiler {
struct RollingSales {
    int recoveries=6;
    double holding_per_hour=.25;
    bool planned_receipts=true;
};
struct SaleTimeline {
    SaleProblem problem;
    int steps[SALE_EVENTS]{};
    bool valid=false, game_end=false;
};
// H6 counts revealed demand recoveries, not hours. Future own flows are used
// only inside the supplied day schedule. The artificial liquidation/holding
// boundary is an explicit baseline, not a calibrated continuation value.
SaleTimeline rolling_sales(const Observation& observation,const History& history,const ResourceSchedule& resources,
                           int product,const Configuration& config={},const RollingSales& options={});
}
