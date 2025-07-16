#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "ast.hpp"
#include "position.hpp"
// removed symbols.hpp
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
inline void to_json(json &j, const Ref<T> &ptr) {
  if (ptr)
    j = *ptr;
  else
    j = nullptr;
}
template <typename T>
inline void from_json(const json &j, Ref<T> &ptr) {
  if (j.is_null())
    ptr = nullptr;
  else {
    auto obj = std::make_shared<T>();
    j.get_to(*obj);
    ptr = obj;
  }
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
  j["name"] = std::string(id.name);
  j["span"] = id.span;
}
inline void from_json(const json &j, Identifier &id) {
  j.at("name").get_to(id.name);
  j.at("span").get_to(id.span);
}

// --- BinaryOp ---
inline void to_json(json &j, const BinaryOp &id) {
  j["left"] = id.left;
  j["right"] = id.right;
  j["op"] = magic_enum::enum_name(id.op);
  j["span"] = id.span;
  j["type"] = "BinaryOp";
}
inline void from_json(const json &j, BinaryOp &id) {
  j.at("left").get_to(id.left);
  j.at("right").get_to(id.right);
  id.op = magic_enum::enum_cast<BinaryOpType>(j.at("op").get<std::string>()).value();
  j.at("span").get_to(id.span);
}

// --- Literal ---
inline void to_json(json &j, const Literal &lit) {
  j["value"] = lit.value;
  j["span"] = lit.span;
  j["type"] = "Literal";
  j["base_type"] = lit.type.base_type;
}
inline void from_json(const json &j, Literal &lit) {
  j.at("value").get_to(lit.value);
  j.at("span").get_to(lit.span);
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

// --- VarDecl ---
inline void to_json(json &j, const VarDecl &l) {
  j = json{{"type", "VarDecl"}};
  j["identifier"] = l.identifier;
  j["expression"] = l.expression;
  j["span"] = l.span;
}
inline void from_json(const json &j, VarDecl &l) {
  j.at("identifier").get_to(l.identifier);
  j.at("expression").get_to(l.expression);
  j.at("span").get_to(l.span);
}

// --- If ---
inline void to_json(json &j, const If &i) {
  j = json{{"type", "If"}};
  j["condition"] = i.condition;
  j["then_branch"] = i.then_branch;
  j["else_branch"] = i.else_branch;
  j["span"] = i.span;
}
inline void from_json(const json &j, If &i) {
  j.at("condition").get_to(i.condition);
  j.at("then_branch").get_to(i.then_branch);
  j.at("else_branch").get_to(i.else_branch);
  j.at("span").get_to(i.span);
}

// --- While ---
inline void to_json(json &j, const While &w) {
  j = json{{"type", "While"}};
  j["condition"] = w.condition;
  j["body"] = w.body;
  j["span"] = w.span;
}
inline void from_json(const json &j, While &w) {
  j.at("condition").get_to(w.condition);
  j.at("body").get_to(w.body);
  j.at("span").get_to(w.span);
}

// --- Block ---
inline void to_json(json &j, const Block &b) {
  j = json{{"type", "Block"}};
  j["statements"] = b.statements;
  j["span"] = b.span;
  j["scope"] = b.scope;
}
inline void from_json(const json &j, Block &b) {
  j.at("statements").get_to(b.statements);
  j.at("span").get_to(b.span);
}


// --- Call ---
inline void to_json(json &j, const Call &c) {
  j = json{{"type", "Call"}};
  j["callee"] = c.callee;
  j["arguments"] = c.arguments;
  j["span"] = c.span;
}
inline void from_json(const json &j, Call &c) {
  j.at("callee").get_to(c.callee);
  j.at("arguments").get_to(c.arguments);
  j.at("span").get_to(c.span);
}

// --- Program ---
inline void to_json(json &j, const Program &p) {
  j = json{{"type", "Program"}};
  j["statements"] = p.statements;
  j["span"] = p.span;
  j["scope"] = p.scope;
}
inline void from_json(const json &j, Program &p) {
  j.at("statements").get_to(p.statements);
  j.at("span").get_to(p.span);
}

// --- ExpressionStatement ---
inline void to_json(json &j, const ExpressionStatement &es) {
  j = json{{"type", "ExpressionStatement"}};
  j["expression"] = es.expression;
  j["span"] = es.span;
}
inline void from_json(const json &j, ExpressionStatement &es) {
  j.at("expression").get_to(es.expression);
  j.at("span").get_to(es.span);
}

// --- Extern ---
inline void to_json(json &j, const Extern &e) {
  j = json{{"type", "Extern"}};
  j["identifier"] = e.identifier;
  j["args"] = json::array();
  for (const auto &arg : e.args) {
    j["args"].push_back(nlohmann::json(*arg));
  }
  j["return_type"] = *e.return_type;
  j["module_path"] = e.module_path;
  j["span"] = e.span;
}
inline void from_json(const json &j, Extern &e) {
  j.at("identifier").get_to(e.identifier);
  e.args.clear();
  for (const auto &arg : j.at("args")) {
    e.args.push_back(std::make_shared<Type>(arg.get<Type>()));
  }
  e.return_type = std::make_shared<Type>(j.at("return_type").get<Type>());
  j.at("module_path").get_to(e.module_path);
  j.at("span").get_to(e.span);
}

// --- Import ---
inline void to_json(json &j, const Import &imp) {
  j = json{{"type", "Import"}};
  j["module_path"] = imp.module_path;
  j["span"] = imp.span;
}
inline void from_json(const json &j, Import &imp) {
  j.at("module_path").get_to(imp.module_path);
  j.at("span").get_to(imp.span);
}

// --- Polymorphic pointer serialization for Expression ---
inline void to_json(json &j, const Ref<Expression> &expr) {
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
  } else if (auto call = std::dynamic_pointer_cast<Call>(expr)) {
    to_json(j, *call);
    j["type"] = "Call";
  } else if (auto bin_op = std::dynamic_pointer_cast<BinaryOp>(expr)) {
    to_json(j, *bin_op);
    j["type"] = "BinaryOp";
  } else {
    throw std::runtime_error("Unknown Expression type for to_json");
  }
}
inline void from_json(const json &j, Ref<Expression> &expr) {
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
    expr = std::static_pointer_cast<Expression>(lit);
  } else if (type == "Call") {
    auto call = std::make_shared<Call>();
    from_json(j, *call);
    expr = call;
  } else if (type == "BinaryOp") {
    auto bin_op = std::make_shared<BinaryOp>();
    from_json(j, *bin_op);
    expr = bin_op;
  } else {
    throw std::runtime_error("Unknown Expression type for from_json: " + type);
  }
}

// --- FunctionDefinition ---
inline void to_json(json &j, const FunctionDefinition &f) {
  j = json{{"type", "FunctionDefinition"}};
  j["identifier"] = f.identifier;
  j["return_type"] = f.return_type;
  j["parameters"] = f.parameters;
  j["returns"] = f.returns;
  j["body"] = f.body;
  j["span"] = f.span;
}
inline void from_json(const json &j, FunctionDefinition &f) {
  j.at("identifier").get_to(f.identifier);
  j.at("return_type").get_to(f.return_type);
  j.at("parameters").get_to(f.parameters);
  j.at("returns").get_to(f.returns);
  j.at("body").get_to(f.body);
  j.at("span").get_to(f.span);
  // Optionally: check type tag
}

// --- Assigment ---
inline void to_json(json &j, const Assigment &a) {
  j = json{{"type", "Assigment"}};
  j["assignee"] = a.assignee;
  j["expression"] = a.expression;
  j["span"] = a.span;
}
inline void from_json(const json &j, Assigment &a) {
  j.at("assignee").get_to(a.assignee);
  j.at("expression").get_to(a.expression);
  j.at("span").get_to(a.span);
}
// --- Polymorphic pointer serialization for Statement ---
inline void to_json(json &j, const Ref<Statement> &stmt) {
  if (!stmt) {
    j = nullptr;
    return;
  }
  if (auto let = std::dynamic_pointer_cast<VarDecl>(stmt)) {
    to_json(j, *let);
    j["type"] = "VarDecl";
  } else if (auto expr_stmt =
                 std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
    to_json(j, *expr_stmt);
    j["type"] = "ExpressionStatement";
  } else if (auto ext = std::dynamic_pointer_cast<Extern>(stmt)) {
    to_json(j, *ext);
    j["type"] = "Extern";
  } else if (auto if_stmt = std::dynamic_pointer_cast<If>(stmt)) {
    to_json(j, *if_stmt);
    j["type"] = "If";
  } else if (auto while_stmt = std::dynamic_pointer_cast<While>(stmt)) {
    to_json(j, *while_stmt);
    j["type"] = "While";
  } else if (auto block = std::dynamic_pointer_cast<Block>(stmt)) {
    to_json(j, *block);
    j["type"] = "Block";
  } else if (auto import_stmt = std::dynamic_pointer_cast<Import>(stmt)) {
    to_json(j, *import_stmt);
    j["type"] = "Import";
  } else if (auto func_def = std::dynamic_pointer_cast<FunctionDefinition>(stmt)) {
    to_json(j, *func_def);
    j["type"] = "FunctionDefinition";
  } else if (auto assign = std::dynamic_pointer_cast<Assigment>(stmt)) {
    to_json(j, *assign);
    j["type"] = "Assigment";
  } else {
    throw std::runtime_error("Unknown Statement type for to_json");
  }
}
inline void from_json(const json &j, Ref<Statement> &stmt) {
  if (j.is_null()) {
    stmt = nullptr;
    return;
  }
  std::string type = j.at("type").get<std::string>();
  if (type == "VarDecl") {
    auto let = std::make_shared<VarDecl>();
    from_json(j, *let);
    stmt = let;
  } else if (type == "ExpressionStatement") {
    auto expr_stmt = std::make_shared<ExpressionStatement>();
    from_json(j, *expr_stmt);
    stmt = expr_stmt;
  } else if (type == "Extern") {
    auto ext = std::make_shared<Extern>();
    from_json(j, *ext);
    stmt = ext;
  } else if (type == "If") {
    auto if_stmt = std::make_shared<If>();
    from_json(j, *if_stmt);
    stmt = if_stmt;
  } else if (type == "Block") {
    auto block = std::make_shared<Block>();
    from_json(j, *block);
    stmt = block;
  } else if (type == "While") {
    auto while_stmt = std::make_shared<While>();
    from_json(j, *while_stmt);
    stmt = while_stmt;
  } else if (type == "Import") {
    auto import_stmt = std::make_shared<Import>();
    from_json(j, *import_stmt);
    stmt = import_stmt;
  } else if (type == "FunctionDefinition") {
    auto func_def = std::make_shared<FunctionDefinition>();
    from_json(j, *func_def);
    stmt = func_def;
  } else if (type == "Assigment") {
    auto assign = std::make_shared<Assigment>();
    from_json(j, *assign);
    stmt = assign;
  } else {
    throw std::runtime_error("Unknown Statement type for from_json: " + type);
  }
}

// --- Return ---
inline void to_json(json &j, const Return &r) {
  j = json{{"type", "Return"}};
  j["expression"] = r.expression;
  j["span"] = r.span;
}
inline void from_json(const json &j, Return &r) {
  j.at("expression").get_to(r.expression);
  j.at("span").get_to(r.span);
}

// --- Symbol ---
inline void to_json(json &j, const Symbol &s) {
  j = json{{"name", s.name}};
  j["symbol_type"] = s.symbol_type;
  j["type"] = s.type;
  j["span"] = s.span;
}
// --- Scope ---
inline void to_json(json &j, const Scope &s) {
  j = json{};
  j["parent"] = nullptr; // avoid recursion
  j["children"] = json::array();
  for (const auto& child : s.children) {
    if (child) j["children"].push_back(*child);
    else j["children"].push_back(nullptr);
  }
  j["symbols"] = json::object();
  for (const auto& [name, symbol] : s.symbols) {
    if (symbol) j["symbols"][name] = *symbol;
    else j["symbols"][name] = nullptr;
  }
}

// --- End of file ---
// Only one to_json/from_json per type in the global namespace. No struct/class
// definitions. No duplicate definitions. All field/method access is correct.
// Polymorphic pointer support is included for Expression/Statement.
