#pragma once
#include <cstdint>

extern "C" {
void* candidate_scoring_create(const int*, const int*, const int*, const int*, const int*, const std::int64_t*, const std::int64_t*, const std::int64_t*);
int candidate_scoring_run(void*, const int*, int, std::int64_t*);
int candidate_route_length(void*, const int*, int);
void candidate_scoring_destroy(void*);
void* deadline_order_create(int, const int*, const int*, const int*, const int*, int, int);
int deadline_order_run(void*, const int*, int, int*);
void deadline_order_destroy(void*);
void* route_propagation_create(int, const int*, const int*, const int*, const int*, const int*, const int*);
int route_propagation_run(void*, const int*, int, const int*, int, const int*, int*, int*);
void route_propagation_destroy(void*);
void* strong_deadline_create(const int*, const int*, const std::int64_t*, const int*, const int*);
void strong_deadline_destroy(void*);
int strong_deadline_run(void*, const int*, int, const int*, int, int*);
int strong_deadline_matching(void*, const int*, int, const int*, int, const int*, int*);
void* neighborhood_create(void*, void*, int, const int*, const int*, const int*);
int neighborhood_run(void*, const int*, int, const int*, int, const int*, int, int, double, int);
const std::int64_t* neighborhood_result(void*, int*);
const char* neighborhood_error(void*);
void neighborhood_destroy(void*);
}
