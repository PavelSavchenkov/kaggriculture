// Internal pipeline bridge only: proposals must come from our cold solver.
#include "adapter.hpp"
int main(int argc, char** argv) { return run_adapter(argc, argv, true); }
