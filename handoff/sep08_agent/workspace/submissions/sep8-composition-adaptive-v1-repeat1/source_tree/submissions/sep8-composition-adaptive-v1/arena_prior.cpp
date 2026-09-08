#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/proposals/animal_repair_q24_premium_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/candidates/investment_context_guarded_001_best/source/agent.hpp"
int main(int argc,char**argv){auto o=compositions::options(argc,argv);return compositions::run_batch(o,[]{return compositions::animal_repair_q24_premium_m2::Agent{};},[]{return compositions::investment_context_guarded_001_best::Agent{};});}
