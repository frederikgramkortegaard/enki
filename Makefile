CC        = g++
OBJ_DIR   = build
# All source files for the core library
CORE_SRCS = $(wildcard src/compiler/*.cpp)
CORE_SRCS += $(wildcard src/runtime/*.cpp)
CORE_SRCS += $(wildcard src/interpreter/*.cpp)
CORE_SRCS += $(wildcard src/definitions/*.cpp)
CORE_SRCS += $(wildcard src/utils/*.cpp)
CORE_OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(CORE_SRCS))

# Driver object file
MORPH_SRC = src/morph.cpp
MORPH_EXE = morph

# Dependency tracking
DEPDIR   := $(OBJ_DIR)/deps
DEPFILES := $(patsubst src/%.cpp,$(DEPDIR)/%.d,$(CORE_SRCS)) $(DEPDIR)/morph.d
DEPFLAGS  = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

# Compiler and linker flags
CFLAGS = -I/opt/homebrew/include -g -std=c++20 -Wall -Wpedantic -I/opt/homebrew/include/nlohmann 
LDFLAGS   = -L/opt/homebrew/lib
LDLIBS    = -lm -lspdlog -lfmt

.PHONY: all clean debug install
all: $(MORPH_EXE)

debug: CFLAGS += -DDEBUG
debug: all

# Rule to create dependency files
$(DEPFILES):
	@mkdir -p "$(@D)"

# Rule to compile source files into object files
$(OBJ_DIR)/%.o : src/%.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo "Compiling $<"
	@mkdir -p "$(@D)"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@ 

# Rule to build the unified executable
$(MORPH_EXE): $(CORE_OBJS) $(MORPH_SRC)
	@echo "Building final executable $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR):
	@mkdir -p $@

$(DEPDIR): 
	@mkdir -p $@

install: $(MORPH_EXE)
	@echo "Installing $(MORPH_EXE) to /usr/local/bin"
	cp $(MORPH_EXE) /usr/local/bin/

clean:
	@echo "Cleaning build artifacts"
	rm -rf $(OBJ_DIR) $(MORPH_EXE)
	rm -f morphir morpheval morphrun

# Include all dependency files for header dependency tracking
-include $(wildcard $(DEPFILES))
