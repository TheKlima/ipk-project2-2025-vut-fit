# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -pedantic -Iinclude
DEPFLAGS = -MMD -MP

# Directories
SRC_DIR = src
OBJ_DIR = obj

# Target executable name
TARGET = ipk25chat-client

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Dependency files
DEPS = $(OBJS:.o=.d)

# Default target
all: $(TARGET)

# Rule to create the target executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Rule to create object files and generate dependencies
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

tcp-serv: pseudo-servers/tcp-serv.c
	gcc $^ -o $@

udp-serv: pseudo-servers/udp-serv.cpp
	$(CXX) $^ -o $@

# Include the dependency files
-include $(DEPS)

pack:
	zip -r xklyme00.zip LICENSE CHANGELOG.md Makefile README.md ./src ./include

# Clean up
clean:
	rm -rf $(OBJ_DIR)
	rm $(TARGET)
	rm tcp-serv
	rm udp-serv

# Phony targets
.PHONY: all clean
