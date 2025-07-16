
#pragma once
#include <ostream>
#include <string_view>
#include <variant>
#include <memory>
#include "position.hpp"
#include "tokens.hpp"

template <typename T>
using Ref = std::shared_ptr<T>;

struct FunctionDefinition;
struct Type;
struct Variable;

struct Function {
    std::string name;
    Span span;
    std::vector<Ref<Variable>> parameters;
    Ref<Type> return_type;
    Ref<FunctionDefinition> definition;
};

enum class BaseType { Int, Float, String, Bool, Identifier, Function };
enum class SymbolType { Function, Variable, Argument };

struct Type {
    BaseType base_type;
    std::variant<Function> structure;
    Span span;
};

struct Variable {
    std::string name;
    Span span;
    Ref<Type> type;
};

struct Symbol {
    std::string name;
    SymbolType symbol_type;
    Ref<Type> type;
    Span span;
};

struct Scope {
  Ref<Scope> parent;
  std::vector<Ref<Scope>> children;
  std::unordered_map<std::string, Ref<Symbol>> symbols;
};