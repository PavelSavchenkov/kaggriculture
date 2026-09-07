#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
int main(int argc,char**argv){auto o=compositions::options(argc,argv);return compositions::run_batch(o,[]{return compositions::shop_herd_s6_m3_g1::Agent{};},[]{return compositions::teammate_shoprouter::Agent{};});}
