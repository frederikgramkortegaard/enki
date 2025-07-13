CC        = g++
OBJ       = build
SRCS = $(wildcard src/*.cpp) $(wildcard src/compiler/*.cpp) $(wildcard src/runtime/*.cpp)
EVAL_SRCS = src/interpreter/eval.cpp src/runtime/impls.cpp
EVAL_EXE = morpheval
DEPDIR   := $(OBJ)/deps
DEPFILES := $(patsubst src/%.cpp,$(DEPDIR)/%.d,$(SRCS))
OBJS      = $(patsubst src/%.cpp,$(OBJ)/%.o,$(SRCS))
DEPFLAGS  = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
EXE       = morphir
CFLAGS = -I/opt/homebrew/include -g -std=c++20 -Wall -Wpedantic -I/opt/homebrew/include/nlohmann 
LDLIBS    = -lm -lstdc++ 

all: $(EXE) $(EVAL_EXE)

debug: CFLAGS += -DDEBUG
debug: all

$(DEPFILES):
	@mkdir -p "$(@D)"

$(EXE): $(OBJS) | $(BIN)
	@echo "($(MODE)) Building final executable $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ)/%.o : src/%.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo "($(MODE)) Compiling $@"
	@mkdir -p "$(@D)"
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@ 

$(OBJ):
	@mkdir -p $@

$(DEPDIR): 
	@mkdir -p $@

clean:
	rm -rf build $(EXE) $(EVAL_EXE)

include $(wildcard $(DEPFILES))

$(EVAL_EXE): $(EVAL_SRCS)
	@echo "Building interpreter executable $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
