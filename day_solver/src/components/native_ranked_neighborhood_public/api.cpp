#include <cstdint>
extern "C" {
void* ranked_neighborhood_create(void*, void*, int, const int*, const int*, const int*);
int ranked_neighborhood_run(void*, const int*, int, const int*, int, const int*, int, int, double, int);
const std::int64_t* ranked_neighborhood_result(void*, int*);
const char* ranked_neighborhood_error(void*);
void ranked_neighborhood_destroy(void*);
void* neighborhood_create(void* a, void* b, int n, const int* ids, const int* offsets, const int* pairs) {
    return ranked_neighborhood_create(a, b, n, ids, offsets, pairs);
}
int neighborhood_run(void* c, const int* p, int n, const int* s, int ns, const int* v, int nv, int k, double seconds, int seeds) {
    return ranked_neighborhood_run(c, p, n, s, ns, v, nv, k, seconds, seeds);
}
const std::int64_t* neighborhood_result(void* c, int* n) { return ranked_neighborhood_result(c, n); }
const char* neighborhood_error(void* c) { return ranked_neighborhood_error(c); }
void neighborhood_destroy(void* c) { ranked_neighborhood_destroy(c); }
}
