#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_greedy_v0/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
using PairA = compositions::greedy::Agent<0>;
using PairB = compositions::public_router::Agent;
