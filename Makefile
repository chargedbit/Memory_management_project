# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = 

# Directories
SRC_DIR = .
ALLOCATOR_DIR = allocator
CACHE_DIR = cache
STATS_DIR = stats
BIN_DIR = bin
OBJ_DIR = obj

# Source files
MAIN_SRC = $(SRC_DIR)/main.cpp
ALLOCATOR_SRC = $(ALLOCATOR_DIR)/MemoryManager.cpp
CACHE_SRC = $(CACHE_DIR)/CacheSimulator.cpp
STATS_SRC = $(STATS_DIR)/StatsManager.cpp

# Object files
MAIN_OBJ = $(OBJ_DIR)/main.o
ALLOCATOR_OBJ = $(OBJ_DIR)/MemoryManager.o
CACHE_OBJ = $(OBJ_DIR)/CacheSimulator.o
STATS_OBJ = $(OBJ_DIR)/StatsManager.o

# All object files
OBJS = $(MAIN_OBJ) $(ALLOCATOR_OBJ) $(CACHE_OBJ) $(STATS_OBJ)

# Executable
TARGET = $(BIN_DIR)/MemoryManagementSimulator

# Default target
all: $(TARGET)

# Create directories if they don't exist
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Compile main.cpp
$(MAIN_OBJ): $(MAIN_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(ALLOCATOR_DIR) -I$(CACHE_DIR) -I$(STATS_DIR) -c $< -o $@

# Compile allocator/MemoryManager.cpp
$(ALLOCATOR_OBJ): $(ALLOCATOR_SRC) $(ALLOCATOR_DIR)/MemoryManager.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(ALLOCATOR_DIR) -c $< -o $@

# Compile cache/CacheSimulator.cpp
$(CACHE_OBJ): $(CACHE_SRC) $(CACHE_DIR)/CacheSimulator.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(CACHE_DIR) -c $< -o $@

# Compile stats/StatsManager.cpp
$(STATS_OBJ): $(STATS_SRC) $(STATS_DIR)/StatsManager.h | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(STATS_DIR) -c $< -o $@

# Link executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Run the executable
run: $(TARGET)
	@echo "Running simulator..."
	./$(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Clean complete."

# Rebuild from scratch
rebuild: clean all

# Phony targets
.PHONY: all run clean rebuild
