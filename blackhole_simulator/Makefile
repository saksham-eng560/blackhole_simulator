# ═══════════════════════════════════════════════════════════════════════════
# Schwarzschild Black Hole Ray Tracer — Simple Makefile
#
# Usage:
#   make              Build the terminal simulator
#   make run          Build and run
#   make clean        Remove build artifacts
#   make debug        Build with debug symbols and sanitizers
# ═══════════════════════════════════════════════════════════════════════════

CXX       = g++
CXXFLAGS  = -std=c++17 -O3 -flto -Wall -Wextra -Wpedantic -march=native -pthread
INCLUDES  = -Iinclude
TARGET    = blackhole_sim

SOURCES   = src/main.cpp           \
            src/accretion_disk.cpp  \
            src/camera.cpp          \
            src/ray_tracer.cpp      \
            src/terminal_renderer.cpp

OBJECTS   = $(SOURCES:.cpp=.o)

# ── Default target ──────────────────────────────────────────────────────
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# ── Pattern rule for .cpp → .o ──────────────────────────────────────────
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# ── Convenience targets ─────────────────────────────────────────────────
.PHONY: run clean debug

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)

debug:
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -fsanitize=address,undefined \
	       $(INCLUDES) -o $(TARGET)_debug $(SOURCES)
