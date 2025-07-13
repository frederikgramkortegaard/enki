#include "../interpreter/eval.hpp"
#include "builtins.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Helper: Read a 24-bit BMP file
struct LoadedImage {
  int width;
  int height;
  std::vector<uint8_t> data;
};
LoadedImage load_bmp(const std::string &filename) {
  std::cout << "Loading BMP file: " << filename << std::endl;
  std::ifstream file(filename, std::ios::binary);
  if (!file)
    throw std::runtime_error("Cannot open file: " + filename);

  uint8_t header[54];
  file.read(reinterpret_cast<char *>(header), 54);
  if (header[0] != 'B' || header[1] != 'M')
    throw std::runtime_error("Not a BMP file");

  int width = *reinterpret_cast<int *>(&header[18]);
  int height = *reinterpret_cast<int *>(&header[22]);
  int offset = *reinterpret_cast<int *>(&header[10]);
  int bpp = *reinterpret_cast<short *>(&header[28]);
  if (bpp != 24)
    throw std::runtime_error("Only 24-bit BMP supported");

  int row_padded = (width * 3 + 3) & (~3);
  std::vector<uint8_t> data(width * height * 3);

  file.seekg(offset, std::ios::beg);
  for (int y = height - 1; y >= 0; --y) {
    file.read(reinterpret_cast<char *>(&data[y * width * 3]), width * 3);
    file.ignore(row_padded - width * 3);
  }
  return {width, height, std::move(data)};
}

// Helper: Write a 24-bit BMP file
void save_bmp(const std::string &filename, const ImageValue &img) {
  int width = img.width, height = img.height;
  int row_padded = (width * 3 + 3) & (~3);
  int filesize = 54 + row_padded * height;
  std::vector<uint8_t> filedata(filesize, 0);

  // BMP header
  filedata[0] = 'B';
  filedata[1] = 'M';
  *reinterpret_cast<int *>(&filedata[2]) = filesize;
  *reinterpret_cast<int *>(&filedata[10]) = 54;
  *reinterpret_cast<int *>(&filedata[14]) = 40;
  *reinterpret_cast<int *>(&filedata[18]) = width;
  *reinterpret_cast<int *>(&filedata[22]) = height;
  *reinterpret_cast<short *>(&filedata[26]) = 1;
  *reinterpret_cast<short *>(&filedata[28]) = 24;

  // Pixel data
  for (int y = height - 1; y >= 0; --y) {
    std::memcpy(&filedata[54 + (height - 1 - y) * row_padded],
                &img.data[y * width * 3], width * 3);
  }

  std::ofstream file(filename, std::ios::binary);
  file.write(reinterpret_cast<const char *>(filedata.data()), filesize);
}

// Example Value type is from eval.hpp

// Builtin function implementations (stubs)
Value builtin_open(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("open(filename): filename must be a string");
  auto str_val = std::dynamic_pointer_cast<StringValue>(args[0]);
  if (!str_val)
    throw std::runtime_error("open(filename): filename must be a string");
  std::string filename = str_val->value;
  auto img = load_bmp(filename);
  std::cout << "Opened image: " << filename << " (" << img.width << "x"
            << img.height << ")\n";
  return std::make_shared<ImageValue>(img.width, img.height,
                                      std::move(img.data), "bmp");
}
Value builtin_save(const std::vector<Value> &args) {
  if (args.size() != 2)
    throw std::runtime_error(
        "save(image, filename): expects image and filename");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  auto str_val = std::dynamic_pointer_cast<StringValue>(args[1]);
  if (!img_val || !str_val)
    throw std::runtime_error(
        "save(image, filename): expects image and filename");
  save_bmp(str_val->value, *img_val);
  std::cout << "Saved image: " << str_val->value << "\n";
  return nullptr;
}
Value builtin_greyscale(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("greyscale(image): expects image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("greyscale(image): expects image");
  auto out = std::make_shared<ImageValue>(*img_val);
  for (size_t i = 0; i + 2 < out->data.size(); i += 3) {
    uint8_t r = out->data[i], g = out->data[i + 1], b = out->data[i + 2];
    uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
    out->data[i] = out->data[i + 1] = out->data[i + 2] = gray;
  }
  std::cout << "Converted image to greyscale\n";
  return out;
}
Value builtin_invert(const std::vector<Value> &args) {
  std::cout << "Called invert\n";
  return nullptr;
}
Value builtin_threshold(const std::vector<Value> &args) {
  std::cout << "Called threshold with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_brightness(const std::vector<Value> &args) {
  std::cout << "Called brightness with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_contrast(const std::vector<Value> &args) {
  std::cout << "Called contrast with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_saturate(const std::vector<Value> &args) {
  std::cout << "Called saturate with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_exposure(const std::vector<Value> &args) {
  std::cout << "Called exposure with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_resize(const std::vector<Value> &args) {
  std::cout << "Called resize with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_rotate(const std::vector<Value> &args) {
  std::cout << "Called rotate with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_flip(const std::vector<Value> &args) {
  std::cout << "Called flip with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_crop(const std::vector<Value> &args) {
  std::cout << "Called crop with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_blur(const std::vector<Value> &args) {
  std::cout << "Called blur with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_sharpen(const std::vector<Value> &args) {
  std::cout << "Called sharpen with " << args.size() << " args\n";
  return nullptr;
}
Value builtin_emboss(const std::vector<Value> &args) {
  std::cout << "Called emboss\n";
  return nullptr;
}
Value builtin_edge_detect(const std::vector<Value> &args) {
  std::cout << "Called edge_detect\n";
  return nullptr;
}
Value builtin_print(const std::vector<Value> &args) {
  std::cout << "Called print with " << args.size() << " args" << std::endl;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i])
      args[i]->print(std::cout);
    else
      std::cout << "<null>";
    if (i + 1 < args.size())
      std::cout << " ";
  }
  std::cout << std::endl;
  return nullptr;
}

std::unordered_map<std::string, BuiltinFunction> builtin_functions;

void register_builtins() {
  builtin_functions = {
      {"open", {"open", 1, 1, builtin_open}},
      {"save", {"save", 2, 2, builtin_save}},
      {"greyscale", {"greyscale", 1, 1, builtin_greyscale}},
      {"print", {"print", 0, 100, builtin_print}},
      {"invert", {"invert", 0, 0, builtin_invert}},
      {"threshold", {"threshold", 1, 1, builtin_threshold}},
      {"brightness", {"brightness", 1, 1, builtin_brightness}},
      {"contrast", {"contrast", 1, 1, builtin_contrast}},
      {"saturate", {"saturate", 1, 1, builtin_saturate}},
      {"exposure", {"exposure", 1, 1, builtin_exposure}},
      {"resize", {"resize", 2, 2, builtin_resize}},
      {"rotate", {"rotate", 1, 1, builtin_rotate}},
      {"flip", {"flip", 1, 1, builtin_flip}},
      {"crop", {"crop", 4, 4, builtin_crop}},
      {"blur", {"blur", 1, 1, builtin_blur}},
      {"sharpen", {"sharpen", 1, 1, builtin_sharpen}},
      {"emboss", {"emboss", 0, 0, builtin_emboss}},
      {"edge_detect", {"edge_detect", 0, 0, builtin_edge_detect}},
  };
}
