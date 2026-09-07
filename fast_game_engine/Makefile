CXX ?= g++
BUILD := build
TRACE ?= traces/replay_kaggriculture-c31-kaito-v22-official-1-32-6_101.txt

CPPFLAGS := -I.
BASE_FLAGS := -std=c++20 -O3 -march=native -mtune=native -DNDEBUG \
	-fno-exceptions -fno-rtti -fno-math-errno -fno-semantic-interposition -fno-plt
RELEASE_FLAGS := $(BASE_FLAGS) -flto

.PHONY: all check extended-check fuzz-check unfinalized-check price-check rng-check \
	coverage-check benchmark \
	comparisons vector-report profile pgo python-baseline clean
all: $(BUILD)/validate $(BUILD)/bench $(BUILD)/layout_bench

$(BUILD):
	mkdir -p $@

$(BUILD)/validate: validate.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) $< -o $@

$(BUILD)/bench: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) $< -o $@

$(BUILD)/layout_bench: layout_bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) $< -o $@

check: $(BUILD)/validate
	find traces -name '*.txt' -print0 | sort -z | xargs -0 $(BUILD)/validate

fuzz-check: $(BUILD)/validate
	conda run -n kaggriculture python differential_fuzz.py traces/fuzz
	find traces/fuzz -name '*.txt' -print0 | sort -z | xargs -0 $(BUILD)/validate

$(BUILD)/validate_unfinalized: validate.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_VALIDATE_UNFINALIZED $< -o $@

unfinalized-check: $(BUILD)/validate_unfinalized
	find traces -name '*.txt' -print0 | sort -z | xargs -0 $(BUILD)/validate_unfinalized

$(BUILD)/price_validate: price_validate.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) $< -o $@

price-check: $(BUILD)/price_validate
	conda run -n kaggriculture --no-capture-output python price_truth.py | $(BUILD)/price_validate

$(BUILD)/rng_validate: rng_validate.cpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) $< -o $@

rng-check: $(BUILD)/rng_validate
	conda run -n kaggriculture --no-capture-output python rng_truth.py | $(BUILD)/rng_validate

coverage-check:
	@if [ -d $(BUILD)/coverage ]; then gio trash $(BUILD)/coverage; fi
	mkdir -p $(BUILD)/coverage
	cd $(BUILD)/coverage && $(CXX) -I../.. -std=c++20 -O0 -g --coverage \
		../../validate.cpp -o validate_cov
	cd $(BUILD)/coverage && find ../../traces -name '*.txt' -print0 | sort -z | \
		xargs -0 ./validate_cov >/dev/null
	cd $(BUILD)/coverage && gcov -b -c validate_cov-validate.gcno > gcov-summary.txt
	grep -A4 "File '../../sim.hpp'" $(BUILD)/coverage/gcov-summary.txt

extended-check: check fuzz-check unfinalized-check price-check rng-check coverage-check

benchmark: $(BUILD)/bench $(BUILD)/layout_bench
	taskset -c 0 $(BUILD)/bench $(TRACE) 5000 7
	taskset -c 0 $(BUILD)/layout_bench

$(BUILD)/bench_no_price_table: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_DISABLE_PRICE_TABLE $< -o $@

$(BUILD)/bench_no_dirty_prices: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_DISABLE_DIRTY_PRICES $< -o $@

$(BUILD)/bench_no_decay_mask: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_DISABLE_DECAY_MASK $< -o $@

$(BUILD)/bench_no_decay_scheduler: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_DISABLE_DECAY_SCHEDULER $< -o $@

$(BUILD)/bench_entity_scans: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_DISABLE_ENTITY_MASKS $< -o $@

$(BUILD)/bench_explicit_avx2: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_EXPLICIT_AVX2 $< -o $@

$(BUILD)/bench_wide_counts: bench.cpp sim.hpp pyrandom.hpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -DKAG_WIDE_COUNTS $< -o $@

comparisons: $(BUILD)/bench $(BUILD)/bench_no_price_table \
	$(BUILD)/bench_no_dirty_prices $(BUILD)/bench_no_decay_mask \
	$(BUILD)/bench_no_decay_scheduler $(BUILD)/bench_entity_scans \
	$(BUILD)/bench_explicit_avx2 $(BUILD)/bench_wide_counts
	for binary in $^; do taskset -c 0 $$binary $(TRACE) 5000 7; done

vector-report: | $(BUILD)
	$(CXX) $(CPPFLAGS) $(BASE_FLAGS) -fopt-info-vec-optimized=$(BUILD)/vectorization.txt \
		bench.cpp -o $(BUILD)/bench_vector_report

profile: | $(BUILD)
	$(CXX) $(CPPFLAGS) $(BASE_FLAGS) -DKAG_PROFILE bench.cpp -o $(BUILD)/bench_profile
	taskset -c 0 $(BUILD)/bench_profile $(TRACE) 1000 1

pgo: | $(BUILD)
	@if [ -d $(BUILD)/pgo-data ]; then gio trash $(BUILD)/pgo-data; fi
	mkdir -p $(BUILD)/pgo-data
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -fprofile-generate=$(BUILD)/pgo-data bench.cpp -o $(BUILD)/bench_pgo
	taskset -c 0 $(BUILD)/bench_pgo $(TRACE) 2000 1
	$(CXX) $(CPPFLAGS) $(RELEASE_FLAGS) -fprofile-use=$(BUILD)/pgo-data \
		-fprofile-correction bench.cpp -o $(BUILD)/bench_pgo

python-baseline:
	conda run -n kaggriculture python python_bench.py $(TRACE) 3 3

clean:
	gio trash $(BUILD)
