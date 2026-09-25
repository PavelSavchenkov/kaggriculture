#pragma once
// Conversion of one replay day to its DayIntent label (designs/day_intent.md).
#include "intent.hpp"

namespace dc10 {

enum class ConvertStatus { Ok, IgnoredLand, IgnoredDiscard, Failed };

struct Conversion {
    ConvertStatus status = ConvertStatus::Ok;
    std::string failure;  // exact schedule pattern when Failed
    Schema schema;
    DayIntent intent;
    int dropped_actions = 0;  // actions dropped by fixed-value rules
};

// states: all 720 replay states; turns: recorded joint actions.
Conversion convert_day(const std::vector<Sim>& states, const std::vector<std::array<Action, 2>>& turns,
                       int seat, int day);
// Same for an arbitrary trajectory: states[0] is dawn, states[steps] the end of the
// day (after tonight's update unless the day is 29); turns[k] drives states[k].
Conversion convert_steps(const Sim* states, const std::array<Action, 2>* turns, int steps, int seat);
// Absolute field differences between two intents on the same schema.
int intent_distance(const Schema& schema, const DayIntent& a, const DayIntent& b);
}
