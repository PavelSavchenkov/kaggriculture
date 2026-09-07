CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -march=native -mtune=native -DNDEBUG \
  -fno-exceptions -fno-rtti -fno-math-errno -fno-semantic-interposition \
  -fno-plt -pthread -I../..

SOURCES := $(shell sed -n 's/.*"\(source\/[^"]*\.cpp\)".*/\1/p' agent.json)
OBJECTS := $(patsubst %.cpp,build/%.o,$(SOURCES))

.PHONY: all strict sanitize clean

all: smoke

smoke: build/tests/smoke.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

strict:
	$(MAKE) clean
	$(MAKE) CXXFLAGS='-std=c++20 -O2 -pthread -I../.. -Wall -Wextra -Wpedantic -Werror' smoke

sanitize:
	$(MAKE) clean
	$(MAKE) CXXFLAGS='-std=c++20 -O1 -g -pthread -I../.. -fsanitize=address,undefined -fno-omit-frame-pointer' smoke

clean:
	rm -rf build smoke
