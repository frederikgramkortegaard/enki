
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

enum class BaseType {
  Void,
  Int,
  Float,
  String,
  Bool,
  Char,
  Identifier,
  Function,
  Enum,
  Unknown, // Assigned by e.g. Parser before Typechecker, if a Type is
           // referenced by an Identifier e.g. the name of an Enum or a Struct
  Any, // Internal type for functions that accept any type (e.g., print)
};
enum class SymbolType { Function, Variable, Argument, Enum };

struct Enum {
  std::string_view name;
  Span span;
  std::unordered_map<std::string_view, Ref<Variable>> members;
};

struct Type {
  BaseType base_type;
  std::variant<Ref<Function>, Ref<Enum>> structure;
  Span span;
  std::string_view name; // Store the type name when base_type is Unknown (e.g., "Color")
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