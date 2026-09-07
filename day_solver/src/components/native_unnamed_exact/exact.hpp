#pragma once
#include "../native_solver_api/exact.hpp"
#include <memory>
namespace day_native::unnamed {
using exact::HintOptions;
using exact::SolveOptions;
using exact::Result;
using exact::ReplayCheck;
// Owns the immutable day; safe to reuse across internal proposals and callers.
class Session {
    struct Impl;
    std::unique_ptr<const Impl> impl_;
public:
    explicit Session(const day_solver::DayProblem&);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    double preparation_seconds() const;
    Result solve(const HintOptions&, const SolveOptions& = {}, int tuning = 1) const;
};
Result solve(const day_solver::DayProblem&, const HintOptions&, const SolveOptions& = {}, int tuning = 1);
}
