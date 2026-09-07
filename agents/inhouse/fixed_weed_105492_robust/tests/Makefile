CXX ?= g++
ROOT := ../../..
SOURCE := ../source
CXXFLAGS := -std=c++20 -O3 -march=native -mtune=native -DNDEBUG \
	-fno-exceptions -fno-rtti -fno-math-errno -fno-semantic-interposition \
	-fno-plt -flto -pthread -I$(ROOT)

verify: verify.cpp $(SOURCE)/agent.cpp $(SOURCE)/core.cpp \
		$(SOURCE)/donor_overlay.cpp $(SOURCE)/reassignment_overlay.cpp \
		$(SOURCE)/residual_overlay.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

evaluate: evaluate.cpp $(SOURCE)/agent.cpp $(SOURCE)/core.cpp \
		$(SOURCE)/donor_overlay.cpp $(SOURCE)/reassignment_overlay.cpp \
		$(SOURCE)/residual_overlay.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f verify evaluate verify_sanitize

.PHONY: clean
