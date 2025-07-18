
#pragma once
#include "position.hpp"
#include "tokens.hpp"
#include <memory>
#include <ostream>
#include <string_view>
#include <variant>

template <typename T> using Ref = std::shared_ptr<T>;

struct FunctionDefinition;
struct Type;
struct Variable;
struct Scope;

struct Function {
  std::string_view name;
  Span span;
  std::vector<Ref<Variable>> parameters;
  Ref<Type> return_type;
  Ref<FunctionDefinition> definition;
  Ref<Scope> scope;
};

enum class BaseType { Void, Int, Float, String, Bool, Char, Identifier, Function };
enum class SymbolType { Function, Variable, Argument };

struct Type {
  BaseType base_type;
  std::variant<Function> structure;
  Span span;
};

struct Variable {
  std::string_view name;
  Span span;
  Ref<Type> type;
};

struct Symbol {
  std::string_view name;
  SymbolType symbol_type;
  Ref<Type> type;
  Span span;
};

struct Scope {
  Ref<Scope> parent;
  std::vector<Ref<Scope>> children;
  std::unordered_map<std::string_view, Ref<Symbol>> symbols;
  std::unordered_map<std::string_view, Ref<Function>> functions;
};