#pragma once
#include "../definitions/ast.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Polymorphic base class for all values
struct ValueBase {
  virtual ~ValueBase() = default;
  virtual std::string type_name() const = 0;
  virtual void print(std::ostream &os) const = 0;
};

using Value = std::shared_ptr<ValueBase>;

struct IntValue : ValueBase {
  int value;
  IntValue(int v) : value(v) {}
  std::string type_name() const override { return "int"; }
  void print(std::ostream &os) const override { os << value; }
};

struct DoubleValue : ValueBase {
  double value;
  DoubleValue(double v) : value(v) {}
  std::string type_name() const override { return "double"; }
  void print(std::ostream &os) const override { os << value; }
};

struct BoolValue : ValueBase {
  bool value;
  BoolValue(bool v) : value(v) {}
  std::string type_name() const override { return "bool"; }
  void print(std::ostream &os) const override { os << (value ? "true" : "false"); }
};

struct StringValue : ValueBase {
  std::string value;
  StringValue(std::string v) : value(std::move(v)) {}
  std::string type_name() const override { return "string"; }
  void print(std::ostream &os) const override { os << '"' << value << '"'; }
};

struct ImageValue : ValueBase {
  int width;
  int height;
  std::vector<uint8_t> data; // RGBRGB...
  std::string format;        // e.g., "bmp", "jpeg"
  ImageValue(int w, int h, std::vector<uint8_t> d, std::string fmt = "bmp")
      : width(w), height(h), data(std::move(d)), format(std::move(fmt)) {}
  std::string type_name() const override { return "image:" + format; }
  void print(std::ostream &os) const override {
    os << "<image " << width << "x" << height << " " << format << ">";
  }
};

struct EvalContext {
  const Program &program;
  std::unordered_map<std::string, Value> values;

  EvalContext(const Program &prog) : program(prog) {}
};

int interpret(EvalContext &ctx);