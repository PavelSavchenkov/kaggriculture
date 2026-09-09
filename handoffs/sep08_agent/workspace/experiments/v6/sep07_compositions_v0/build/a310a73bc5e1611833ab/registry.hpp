#pragma once
#include <memory>
#include <string>
#include "evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/composition_greedy_v1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/john_131/source/agent.hpp"
using PairA = compositions::john_131::Agent;
using PairB = compositions::greedy_v1::Agent<4,true,true,true,1,true,1>;
