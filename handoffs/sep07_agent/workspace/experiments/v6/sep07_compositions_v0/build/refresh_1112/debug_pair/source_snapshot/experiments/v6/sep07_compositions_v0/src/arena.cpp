#include "evaluation.hpp"
#include "registry.hpp"

int main(int argc, char** argv) {
    const auto o = compositions::options(argc, argv);
#ifdef COMPOSITIONS_PAIR
    return compositions::run_batch(o, [] { return PairA{}; }, [] { return PairB{}; });
#else
    return compositions::run_batch(o, [&] { return make_agent(o.a); }, [&] { return make_agent(o.b); });
#endif
}
