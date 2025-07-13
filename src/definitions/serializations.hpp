#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "ast.hpp"
#include "location.hpp"
#include "span.hpp"
#include "symbols.hpp"
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
// ... include other relevant headers as needed ...

// --- String view helpers ---
inline std::vector<std::string> _interned_strings;
inline void to_json(json &j, const std::string_view &sv) {
  j = std::string(sv);
}
inline void from_json(const json &j, std::string_view &sv) {
  _interned_strings.emplace_back(j.get<std::string>());
  sv = _interned_strings.back();
}

// --- shared_ptr helpers for concrete types ---
template <typename T>
inline void to_json(json &j, const std::shared_ptr<T> &ptr) {
  if (ptr)
    j = *ptr;
  else
    j = nullptr;
}
template <typename T>
inline void from_json(const json &j, std::shared_ptr<T> &ptr) {
  if (j.is_null())
    ptr = nullptr;
  else
    ptr = std::make_shared<T>(j.get<T>());
}

// --- Location ---
inline void to_json(json &j, const Location &loc) {
  j["row"] = loc.row;
  j["col"] = loc.col;
  j["pos"] = loc.pos;
  j["file_name"] = loc.file_name;
}
inline void from_json(const json &j, Location &loc) {
  j.at("row").get_to(loc.row);
  j.at("col").get_to(loc.col);
  j.at("pos").get_to(loc.pos);
  j.at("file_name").get_to(loc.file_name);
}

// --- Span ---
inline void to_json(json &j, const Span &s) {
  j["start"] = s.start;
  j["end"] = s.end;
}
inline void from_json(const json &j, Span &s) {
  j.at("start").get_to(s.start);
  j.at("end").get_to(s.end);
}

// --- Identifier ---
inline void to_json(json &j, const Identifier &id) {
  j["name"] = id.name;
  j["span"] = id.span();
}
inline void from_json(const json &j, Identifier &id) {
  j.at("name").get_to(id.name);
  j.at("span").get_to(id.span());
}

// --- Literal ---
inline void to_json(json &j, const Literal &lit) {
  j["value"] = lit.value;
  j["span"] = lit.span();
  j["type"] = "Literal";
  j["base_type"] = lit.type.base_type;
}
inline void from_json(const json &j, Literal &lit) {
  j.at("value").get_to(lit.value);
  j.at("span").get_to(lit.span());
  lit.type.base_type = j.at("base_type").get<BaseType>();
}

// --- BaseType (enum) ---
inline void to_json(json &j, const BaseType &t) {
  j = std::string(magic_enum::enum_name(t));
}
inline void from_json(const json &j, BaseType &t) {
  t = magic_enum::enum_cast<BaseType>(j.get<std::string>()).value();
}

// --- Type (new, from types.hpp) ---
inline void to_json(json &j, const Type &t) {
  j["base_type"] = t.base_type;
  // TODO: serialize more fields if added
}
inline void from_json(const json &j, Type &t) {
  j.at("base_type").get_to(t.base_type);
  // TODO: deserialize more fields if added
}

// --- LetStatement ---
inline void to_json(json &j, const LetStatement &l) {
  j["identifier"] = l.identifier;
  j["expression"] = l.expression;
  j["span"] = l.span();
  j["type"] = "LetStatement";
}
inline void from_json(const json &j, LetStatement &l) {
  j.at("identifier").get_to(l.identifier);
  j.at("expression").get_to(l.expression);
  j.at("span").get_to(l.span());
}

// --- Symbol ---
inline void to_json(json &j, const Symbol &s) {
  j["name"] = s.name;
  j["type"] = s.type;
  j["span"] = s.span;
}
inline void from_json(const json &j, Symbol &s) {
  j.at("name").get_to(s.name);
  j.at("type").get_to(s.type);
  j.at("span").get_to(s.span);
}

// --- CallExpression ---
inline void to_json(json &j, const CallExpression &c) {
  j["callee"] = c.callee;
  j["arguments"] = c.arguments;
  j["span"] = c.span();
  j["type"] = "CallExpression";
}
inline void from_json(const json &j, CallExpression &c) {
  j.at("callee").get_to(c.callee);
  j.at("arguments").get_to(c.arguments);
  j.at("span").get_to(c.span());
}

// --- Program ---
inline void to_json(json &j, const Program &p) {
  j["statements"] = p.statements;
  j["symbols"] = p.symbols;
  j["span"] = p.span();
  j["type"] = "Program";
}
inline void from_json(const json &j, Program &p) {
  j.at("statements").get_to(p.statements);
  j.at("symbols").get_to(p.symbols);
  j.at("span").get_to(p.span());
}

// --- ExpressionStatement ---
inline void to_json(json &j, const ExpressionStatement &es) {
  j["expression"] = es.expression;
  j["span"] = es.span();
  j["type"] = "ExpressionStatement";
}
inline void from_json(const json &j, ExpressionStatement &es) {
  j.at("expression").get_to(es.expression);
  j.at("span").get_to(es.span());
}

// --- ExternStatement ---
inline void to_json(json &j, const ExternStatement &e) {
  j["identifier"] = e.identifier;
  j["args"] = json::array();
  for (const auto &arg : e.args) {
    j["args"].push_back(*arg);
  }
  j["return_type"] = *e.return_type;
  j["module_path"] = e.module_path;
  j["span"] = e.span();
  j["type"] = "ExternStatement";
}
inline void from_json(const json &j, ExternStatement &e) {
  j.at("identifier").get_to(e.identifier);
  e.args.clear();
  for (const auto &arg : j.at("args")) {
    e.args.push_back(std::make_shared<Type>(arg.get<Type>()));
  }
  e.return_type = std::make_shared<Type>(j.at("return_type").get<Type>());
  j.at("module_path").get_to(e.module_path);
  j.at("span").get_to(e.span());
}

// --- Polymorphic pointer serialization for Expression ---
inline void to_json(json &j, const std::shared_ptr<Expression> &expr) {
  if (!expr) {
    j = nullptr;
    return;
  }
  if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
    to_json(j, *ident);
    j["type"] = "Identifier";
  } else if (auto lit = std::dynamic_pointer_cast<Literal>(expr)) {
    to_json(j, *lit);
    j["type"] = "Literal";
  } else if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
    to_json(j, *call);
    j["type"] = "CallExpression";
  } else {
    throw std::runtime_error("Unknown Expression type for to_json");
  }
}
inline void from_json(const json &j, std::shared_ptr<Expression> &expr) {
  if (j.is_null()) {
    expr = nullptr;
    return;
  }
  std::string type = j.at("type").get<std::string>();
  if (type == "Identifier") {
    auto ident = std::make_shared<Identifier>();
    from_json(j, *ident);
    expr = ident;
  } else if (type == "Literal") {
    auto lit = std::make_shared<Literal>();
    from_json(j, *lit);
    expr = lit;
  } else if (type == "CallExpression") {
    auto call = std::make_shared<CallExpression>();
    from_json(j, *call);
    expr = call;
  } else {
    throw std::runtime_error("Unknown Expression type for from_json: " + type);
  }
}

// --- Polymorphic pointer serialization for Statement ---
inline void to_json(json &j, const std::shared_ptr<Statement> &stmt) {
  if (!stmt) {
    j = nullptr;
    return;
  }
  if (auto let = std::dynamic_pointer_cast<LetStatement>(stmt)) {
    to_json(j, *let);
    j["type"] = "LetStatement";
  } else if (auto expr_stmt =
                 std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    to_json(j, *expr_stmt);
    j["type"] = "ExpressionStatement";
  } else if (auto ext = std::dynamic_pointer_cast<ExternStatement>(stmt)) {
    to_json(j, *ext);
    j["type"] = "ExternStatement";
  } else {
    throw std::runtime_error("Unknown Statement type for to_json");
  }
}
inline void from_json(const json &j, std::shared_ptr<Statement> &stmt) {
  if (j.is_null()) {
    stmt = nullptr;
    return;
  }
  std::string type = j.at("type").get<std::string>();
  if (type == "LetStatement") {
    auto let = std::make_shared<LetStatement>();
    from_json(j, *let);
    stmt = let;
  } else if (type == "ExpressionStatement") {
    auto expr_stmt = std::make_shared<ExpressionStatement>();
    from_json(j, *expr_stmt);
    stmt = expr_stmt;
  } else if (type == "ExternStatement") {
    auto ext = std::make_shared<ExternStatement>();
    from_json(j, *ext);
    stmt = ext;
  } else {
    throw std::runtime_error("Unknown Statement type for from_json: " + type);
  }
}

// --- End of file ---
// Only one to_json/from_json per type in the global namespace. No struct/class
// definitions. No duplicate definitions. All field/method access is correct.
// Polymorphic pointer support is included for Expression/Statement.
