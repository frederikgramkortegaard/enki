CC        = g++
OBJ_DIR   = build
# All source files for the core library
CORE_SRCS = $(wildcard src/compiler/*.cpp) $(wildcard src/runtime/*.cpp) $(wildcard src/interpreter/*.cpp)
CORE_OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(CORE_SRCS))

DEPDIR   := $(OBJ_DIR)/deps
DEPFILES := $(patsubst src/%.cpp,$(DEPDIR)/%.d,$(CORE_SRCS))
DEPFLAGS  = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
CFLAGS = -I/opt/homebrew/include -g -std=c++20 -Wall -Wpedantic -I/opt/homebrew/include/nlohmann 
LDLIBS    = -lm

.PHONY: all clean debug
all: morpheval morphir morphrun

debug: CFLAGS += -DDEBUG
debug: all

# Rule to create dependency files
$(DEPFILES):
	@mkdir -p "$(@D)"

# Rule to compile core source files into object files
$(OBJ_DIR)/%.o : src/%.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo "($(MODE)) Compiling $<"
	@mkdir -p "$(@D)"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@ 

# Rules to build executables
morphir: ./src/drivers/morphir.cpp $(CORE_OBJS)
	@echo "Building final executable $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

morpheval: ./src/drivers/morpheval.cpp $(CORE_OBJS)
	@echo "Building final executable $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

morphrun: ./src/drivers/morphrun.cpp $(CORE_OBJS)
	@echo "Building final executable $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ_DIR):
	@mkdir -p $@

$(DEPDIR): 
	@mkdir -p $@

clean:
	rm -rf build $(MORPHIR_EXE) $(MORPHEVAL_EXE) $(MORPHRUN_EXE)

include $(wildcard $(DEPFILES))
