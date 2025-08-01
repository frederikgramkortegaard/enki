
#include "types.hpp"

#include <fmt/format.h>
#include <iostream>

bool types_are_equal(Ref<Type> dest, Ref<Type> src) {
  if (!dest || !src)
    return false;

  if (dest->base_type == BaseType::Any) {
    return true;
  }
  if (dest->base_type != src->base_type)
    return false;

  if (src->base_type == BaseType::Enum) {
    auto left_enum = std::get<Ref<Enum>>(dest->structure);
    auto right_enum = std::get<Ref<Enum>>(src->structure);
    return left_enum->name == right_enum->name;
  }

  // If the source is a pointer, we can only assign it to a compatible type
  if (src->base_type == BaseType::Pointer) {
    auto left_ptr = std::get<Ref<Type>>(dest->structure);
    auto right_ptr = std::get<Ref<Type>>(src->structure);
    return types_are_equal(left_ptr, right_ptr);
  }


   // Struct testing, probably just nominative for now
  if (dest->base_type == BaseType::Struct) {
    auto left_struct = std::get<Ref<Struct>>(dest->structure);
    auto right_struct = std::get<Ref<Struct>>(src->structure);
    return left_struct->name == right_struct->name;
  }

  return true;
}

bool can_assign_type(Ref<Type> left, Ref<Type> right) {
  // TODO: Allow some implicit conversions, helpful for pointer-types especially
  return types_are_equal(left, right);
}

// Overloaded version that takes context about the expression
bool can_assign_type_with_context(Ref<Type> left, Ref<Type> right, bool is_type_reference) {
  // If left is BaseType::Type, only accept type references
  if (left->base_type == BaseType::Type) {
    return is_type_reference;
  }
  
  // Otherwise, use normal type equality
  return types_are_equal(left, right);
}

std::string Type::to_string() const {
  std::string result;
  switch (base_type) {
  case BaseType::Void:
    result = "void";
    break;
  case BaseType::Int:
    result = "int";
    break;
  case BaseType::Float:
    result = "float";
    break;
  case BaseType::String:
    result = "String";
    break;
  case BaseType::Bool:
    result = "bool";
    break;
  case BaseType::Char:
    result = "char";
    break;
  case BaseType::Type:
    result = "Type";
    break;
  case BaseType::Identifier:
    result = name;
    break;
  case BaseType::Function:
    result = std::get<Ref<Function>>(structure)->name;
    break;
  case BaseType::Enum:
    result = std::get<Ref<Enum>>(structure)->name;
    break;
  case BaseType::Struct:
    result = std::get<Ref<Struct>>(structure)->name;
    break;
  case BaseType::Pointer:
    result = "&" + std::get<Ref<Type>>(structure)->to_string();
    break;
  case BaseType::Unknown:
    if (name.empty()) {
      result = "<Unknown>";
    } else {
      result = fmt::format("<Unknown: {}>", name);
    }
    break;
  default:
    result = magic_enum::enum_name(base_type);
    break;
  }
  return result;
}