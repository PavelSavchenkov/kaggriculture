CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O3 -march=native -mtune=native -DNDEBUG -Wall -Wextra
INCLUDES  = -I include -I ..

all: build_verify

build_verify: verify_corpus.cpp include/corpus/episode.hpp ../fast_game_engine/sim.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) verify_corpus.cpp -o $@ -lz

# Full C++ gate: load every container and check the parity hash of every state.
check: build_verify
	./build_verify data/*.kagz

# Python gates: re-replay through the official engine, and rebuild the source JSON.
check-python:
	ls data/*.kagz | xargs -P 8 -n 1 python3 kagz.py verify

check-roundtrip:
	ls data/*.kagz | xargs -P 4 -n 1 python3 kagz.py verify --roundtrip

# Cross-engine gate: export traces and replay them with the reference validator.
check-trace:
	ls data/*.kagz | xargs -P 8 -n 1 python3 kagz.py trace -o traces
	$(MAKE) -C ../fast_game_engine
	../fast_game_engine/build/validate traces/*.txt

check-all: check check-python check-roundtrip check-trace

clean:
	rm -f build_verify

.PHONY: all check check-python check-roundtrip check-trace check-all clean
