#include <cstdint>
extern "C" {
void* compact_neighborhood_create(void*, void*, int, const int*, const int*, const int*);
int compact_neighborhood_run(void*, const int*, int, const int*, int, const int*, int, int, double, int);
const std::int64_t* compact_neighborhood_result(void*, int*);
const char* compact_neighborhood_error(void*);
void compact_neighborhood_destroy(void*);
void* neighborhood_create(void* a, void* b, int n, const int* ids, const int* offsets, const int* pairs) {
    return compact_neighborhood_create(a, b, n, ids, offsets, pairs);
}
int neighborhood_run(void* c, const int* p, int n, const int* s, int ns, const int* v, int nv, int k, double seconds, int seeds) {
    return compact_neighborhood_run(c, p, n, s, ns, v, nv, k, seconds, seeds);
}
const std::int64_t* neighborhood_result(void* c, int* n) { return compact_neighborhood_result(c, n); }
const char* neighborhood_error(void* c) { return compact_neighborhood_error(c); }
void neighborhood_destroy(void* c) { compact_neighborhood_destroy(c); }
}
