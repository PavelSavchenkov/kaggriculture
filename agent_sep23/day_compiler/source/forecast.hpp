#pragma once
#include "history.hpp"
#include <array>

namespace kag::day_compiler {
using FlowFeatures=std::array<double,24>;
FlowFeatures flow_features(const Observation& observation,const History& history,int product,int horizon,
                           const Configuration& config={});
double predict_flow(int product,const FlowFeatures& features);
// A monotone cumulative point forecast defines one coherent nonnegative path.
// It is not a calibrated uncertainty model or knowledge of future rival trades.
void forecast_sales(const Observation& observation,const History& history,int product,int hours,int* sales,
                    const Configuration& config={},bool terminal_balance=false);
// A causal funding stress path: public ripe output may arrive as early as its
// isolated transport bound, and inferred pending stock may sell immediately.
// This covers an explicit scenario, not every hidden inventory or rival action.
void cover_visible_supply(const Observation& observation,const History& history,int product,int hours,int* sales,
                          const Configuration& config={});
}
