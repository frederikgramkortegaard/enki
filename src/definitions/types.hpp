
#pragma once
#include "position.hpp"
#include "tokens.hpp"
#include <memory>
#include <ostream>
#include <string_view>
#include <variant>

template <typename T> using Ref = std::shared_ptr<T>;

struct StructDefinition;
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
  Pointer,
  Struct,
  Unknown, // Assigned by e.g. Parser before Typechecker, if a Type is
           // referenced by an Identifier e.g. the name of an Enum or a Struct
  Type, // This is a meta type for types, e.g. the type of a type
  Any, // Internal type for functions that accept any type (e.g., print)
};
enum class SymbolType { Function, Variable, Argument, Enum, Struct };

struct Enum {
  std::string_view name;
  Span span;
  std::unordered_map<std::string_view, Ref<Variable>> members;
};

struct Struct {
  std::string_view name;
  Span span;
  Ref<StructDefinition> definition;
  std::vector<Ref<Variable>> fields;
};

struct Type {
  BaseType base_type;
  std::variant<Ref<Function>, Ref<Enum>, Ref<Struct>, Ref<Type>> structure;
  Span span;
  std::string_view name; // Store the type name when base_type is Unknown (e.g., "Color")

  std::string to_string() const;
};

bool types_are_equal(Ref<Type> left, Ref<Type> right);
bool can_assign_type(Ref<Type> left, Ref<Type> right);
bool can_assign_type_with_context(Ref<Type> left, Ref<Type> right, bool is_type_reference);

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
};