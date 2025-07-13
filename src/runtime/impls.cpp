#include "../interpreter/eval.hpp"
#include "builtins.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>

// Helper: Read a 24-bit BMP file
struct LoadedImage {
  int width;
  int height;
  std::vector<uint8_t> data;
};
LoadedImage load_bmp(const std::string &filename) {
  spdlog::debug("Loading BMP file: {}", filename);
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
  spdlog::debug("Opened image: {} ({}x{})", filename, img.width, img.height);
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
  std::string filename = str_val->value;
  save_bmp(filename, *img_val);
  spdlog::debug("Saved image to: {}", filename);
  return nullptr;
}

Value builtin_greyscale(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("greyscale(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("greyscale(image): expects an image");

  spdlog::debug("Applying greyscale to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  for (size_t i = 0; i < new_img->data.size(); i += 3) {
    uint8_t r = new_img->data[i], g = new_img->data[i + 1], b = new_img->data[i + 2];
    uint8_t grey = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
    new_img->data[i] = new_img->data[i + 1] = new_img->data[i + 2] = grey;
  }
  return new_img;
}

Value builtin_blur(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("blur(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("blur(image): expects an image");

  spdlog::debug("Applying blur to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  std::vector<uint8_t> temp_data = new_img->data;

  int width = new_img->width, height = new_img->height;
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      for (int c = 0; c < 3; ++c) {
        int sum = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            sum += temp_data[3 * ((y + dy) * width + (x + dx)) + c];
        new_img->data[3 * (y * width + x) + c] = static_cast<uint8_t>(sum / 9);
      }
    }
  }
  return new_img;
}

Value builtin_invert(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("invert(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("invert(image): expects an image");

  spdlog::debug("Applying invert to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  for (size_t i = 0; i < new_img->data.size(); ++i) {
    new_img->data[i] = 255 - new_img->data[i];
  }
  return new_img;
}

Value builtin_flip(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("flip(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("flip(image): expects an image");

  spdlog::debug("Applying flip to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  int width = new_img->width;
  int height = new_img->height;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      for (int c = 0; c < 3; ++c) {
        new_img->data[3 * (y * width + x) + c] =
            img_val->data[3 * (y * width + (width - 1 - x)) + c];
      }
    }
  }
  return new_img;
}

Value builtin_crop(const std::vector<Value> &args) {
  if (args.size() != 5)
    throw std::runtime_error(
        "crop(image, x, y, width, height): expects 5 arguments");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  auto x_val = std::dynamic_pointer_cast<IntValue>(args[1]);
  auto y_val = std::dynamic_pointer_cast<IntValue>(args[2]);
  auto w_val = std::dynamic_pointer_cast<IntValue>(args[3]);
  auto h_val = std::dynamic_pointer_cast<IntValue>(args[4]);

  if (!img_val || !x_val || !y_val || !w_val || !h_val)
    throw std::runtime_error("crop: invalid argument types");

  spdlog::debug("Applying crop to image");
  int x = x_val->value;
  int y = y_val->value;
  int w = w_val->value;
  int h = h_val->value;

  std::vector<uint8_t> new_data(w * h * 3);
  for (int j = 0; j < h; ++j) {
    for (int i = 0; i < w; ++i) {
      for (int c = 0; c < 3; ++c) {
        new_data[3 * (j * w + i) + c] =
            img_val->data[3 * ((y + j) * img_val->width + (x + i)) + c];
      }
    }
  }
  return std::make_shared<ImageValue>(w, h, std::move(new_data), img_val->format);
}

Value builtin_brightness(const std::vector<Value> &args) {
  if (args.size() != 2)
    throw std::runtime_error("brightness(image, factor): expects 2 arguments");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  auto factor_val = std::dynamic_pointer_cast<DoubleValue>(args[1]);
  if (!img_val || !factor_val)
    throw std::runtime_error("brightness: invalid argument types");

  spdlog::debug("Applying brightness to image");
  double factor = factor_val->value;
  auto new_img = std::make_shared<ImageValue>(*img_val);
  for (size_t i = 0; i < new_img->data.size(); ++i) {
    int val = static_cast<int>(new_img->data[i] * factor);
    new_img->data[i] = static_cast<uint8_t>(std::clamp(val, 0, 255));
  }
  return new_img;
}

Value builtin_contrast(const std::vector<Value> &args) {
  if (args.size() != 2)
    throw std::runtime_error("contrast(image, factor): expects 2 arguments");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  auto factor_val = std::dynamic_pointer_cast<DoubleValue>(args[1]);
  if (!img_val || !factor_val)
    throw std::runtime_error("contrast: invalid argument types");

  spdlog::debug("Applying contrast to image");
  double factor = factor_val->value;
  auto new_img = std::make_shared<ImageValue>(*img_val);
  for (size_t i = 0; i < new_img->data.size(); ++i) {
    int val = static_cast<int>((new_img->data[i] - 128) * factor + 128);
    new_img->data[i] = static_cast<uint8_t>(std::clamp(val, 0, 255));
  }
  return new_img;
}

Value builtin_sharpen(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("sharpen(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("sharpen(image): expects an image");

  spdlog::debug("Applying sharpen to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  std::vector<uint8_t> temp_data = new_img->data;
  int width = new_img->width, height = new_img->height;

  int kernel[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      for (int c = 0; c < 3; ++c) {
        int sum = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            sum += kernel[dy + 1][dx + 1] * temp_data[3 * ((y + dy) * width + (x + dx)) + c];
        sum = std::clamp(sum, 0, 255);
        new_img->data[3 * (y * width + x) + c] = static_cast<uint8_t>(sum);
      }
    }
  }
  return new_img;
}

Value builtin_edge_detect(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("edge_detect(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("edge_detect(image): expects an image");

  spdlog::debug("Applying edge detection to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  std::vector<uint8_t> temp_data = new_img->data;
  int width = new_img->width, height = new_img->height;

  int kernel[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      for (int c = 0; c < 3; ++c) {
        int sum = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            sum += kernel[dy + 1][dx + 1] * temp_data[3 * ((y + dy) * width + (x + dx)) + c];
        sum = std::clamp(sum, 0, 255);
        new_img->data[3 * (y * width + x) + c] = static_cast<uint8_t>(sum);
      }
    }
  }
  return new_img;
}

Value builtin_emboss(const std::vector<Value> &args) {
  if (args.size() != 1)
    throw std::runtime_error("emboss(image): expects an image");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  if (!img_val)
    throw std::runtime_error("emboss(image): expects an image");

  spdlog::debug("Applying emboss to image");
  auto new_img = std::make_shared<ImageValue>(*img_val);
  std::vector<uint8_t> temp_data = new_img->data;
  int width = new_img->width, height = new_img->height;

  int kernel[3][3] = {{-2, -1, 0}, {-1, 1, 1}, {0, 1, 2}};
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      for (int c = 0; c < 3; ++c) {
        int sum = 128;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            sum += kernel[dy + 1][dx + 1] * temp_data[3 * ((y + dy) * width + (x + dx)) + c];
        sum = std::clamp(sum, 0, 255);
        new_img->data[3 * (y * width + x) + c] = static_cast<uint8_t>(sum);
      }
    }
  }
  return new_img;
}

Value builtin_exposure(const std::vector<Value> &args) {
  if (args.size() != 2)
    throw std::runtime_error("exposure(image, factor): expects 2 arguments");
  auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
  auto factor_val = std::dynamic_pointer_cast<DoubleValue>(args[1]);
  if (!img_val || !factor_val)
    throw std::runtime_error("exposure: invalid argument types");

  spdlog::debug("Applying exposure to image");
  double factor = factor_val->value;
  auto new_img = std::make_shared<ImageValue>(*img_val);
  for (size_t i = 0; i < new_img->data.size(); ++i) {
    int val = static_cast<int>(new_img->data[i] * factor);
    new_img->data[i] = static_cast<uint8_t>(std::clamp(val, 0, 255));
  }
  return new_img;
}

Value builtin_resize(const std::vector<Value>& args) {
    if (args.size() != 2 && args.size() != 1)
        throw std::runtime_error("resize(image[, ...]): expects at least image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("resize(image): expects image");
    // For now, just return a copy (no actual resize)
    spdlog::debug("Stub: resize returns copy of image");
    return std::make_shared<ImageValue>(*img_val);
}
Value builtin_rotate(const std::vector<Value>& args) {
    if (args.size() != 2)
        throw std::runtime_error("rotate(image, angle): expects image and angle");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    auto angle_val = std::dynamic_pointer_cast<IntValue>(args[1]);
    if (!img_val || !angle_val)
        throw std::runtime_error("rotate(image, angle): expects image and integer angle");
    int angle = angle_val->value;
    double radians = angle * M_PI / 180.0;
    int w = img_val->width, h = img_val->height;

    // Calculate new image size
    double cos_a = std::abs(std::cos(radians));
    double sin_a = std::abs(std::sin(radians));
    int new_w = static_cast<int>(w * cos_a + h * sin_a);
    int new_h = static_cast<int>(w * sin_a + h * cos_a);

    std::vector<uint8_t> out_data(new_w * new_h * 3, 0);

    // Center of original and new image
    double cx = w / 2.0, cy = h / 2.0;
    double ncx = new_w / 2.0, ncy = new_h / 2.0;

    for (int y = 0; y < new_h; ++y) {
        for (int x = 0; x < new_w; ++x) {
            // Map (x, y) in output to (src_x, src_y) in input
            double dx = x - ncx;
            double dy = y - ncy;
            double src_x =  cos(radians) * dx + sin(radians) * dy + cx;
            double src_y = -sin(radians) * dx + cos(radians) * dy + cy;
            int isx = static_cast<int>(std::round(src_x));
            int isy = static_cast<int>(std::round(src_y));
            if (isx >= 0 && isx < w && isy >= 0 && isy < h) {
                for (int c = 0; c < 3; ++c) {
                    out_data[3 * (y * new_w + x) + c] = img_val->data[3 * (isy * w + isx) + c];
                }
            }
            // else: leave black
        }
    }
    spdlog::debug("Rotated image by {} degrees", angle);
    return std::make_shared<ImageValue>(new_w, new_h, std::move(out_data), img_val->format);
}

Value builtin_threshold(const std::vector<Value>& args) {
    if (args.size() != 2)
        throw std::runtime_error("threshold(image, value): expects image and value");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    auto threshold_val = std::dynamic_pointer_cast<IntValue>(args[1]);
    if (!img_val || !threshold_val)
        throw std::runtime_error("threshold: invalid argument types");

    spdlog::debug("Applying threshold to image");
    int threshold = threshold_val->value;
    auto new_img = std::make_shared<ImageValue>(*img_val);
    for (size_t i = 0; i < new_img->data.size(); i += 3) {
        uint8_t r = new_img->data[i], g = new_img->data[i + 1], b = new_img->data[i + 2];
        uint8_t grey = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
        uint8_t new_val = grey > threshold ? 255 : 0;
        new_img->data[i] = new_img->data[i + 1] = new_img->data[i + 2] = new_val;
    }
    return new_img;
}

Value builtin_saturate(const std::vector<Value>& args) {
    if (args.size() != 2)
        throw std::runtime_error("saturate(image, factor): expects image and factor");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    auto factor_val = std::dynamic_pointer_cast<DoubleValue>(args[1]);
    if (!img_val || !factor_val)
        throw std::runtime_error("saturate: invalid argument types");

    spdlog::debug("Applying saturation to image");
    double factor = factor_val->value;
    auto new_img = std::make_shared<ImageValue>(*img_val);
    for (size_t i = 0; i < new_img->data.size(); i += 3) {
        float r = new_img->data[i];
        float g = new_img->data[i+1];
        float b = new_img->data[i+2];
        float gray = 0.299 * r + 0.587 * g + 0.114 * b;
        new_img->data[i] = std::clamp(int(gray + (r - gray) * factor), 0, 255);
        new_img->data[i+1] = std::clamp(int(gray + (g - gray) * factor), 0, 255);
        new_img->data[i+2] = std::clamp(int(gray + (b - gray) * factor), 0, 255);
    }
    return new_img;
}

Value builtin_print(const std::vector<Value> &args) {
  spdlog::debug("Called print with {} args", args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i]) {
      std::stringstream ss;
      args[i]->print(ss);
      fmt::println("{}", ss.str());
    } else {
      fmt::println("<null>");
    }
  }
  return nullptr;
}

std::unordered_map<std::string, BuiltinFunction> builtin_functions;

void register_builtins() {
  builtin_functions = {
      {"open", {"open", 1, 1, builtin_open}},
      {"save", {"save", 2, 2, builtin_save}},
      {"greyscale", {"greyscale", 1, 1, builtin_greyscale}},
      {"print", {"print", 0, 100, builtin_print}},
      {"invert", {"invert", 1, 1, builtin_invert}},
      {"threshold", {"threshold", 2, 2, builtin_threshold}},
      {"brightness", {"brightness", 2, 2, builtin_brightness}},
      {"contrast", {"contrast", 2, 2, builtin_contrast}},
      {"saturate", {"saturate", 2, 2, builtin_saturate}},
      {"exposure", {"exposure", 2, 2, builtin_exposure}},
      {"resize", {"resize", 3, 3, builtin_resize}},
      {"rotate", {"rotate", 2, 2, builtin_rotate}},
      {"flip", {"flip", 1, 1, builtin_flip}},
      {"crop", {"crop", 5, 5, builtin_crop}},
      {"blur", {"blur", 1, 1, builtin_blur}},
      {"sharpen", {"sharpen", 1, 1, builtin_sharpen}},
      {"emboss", {"emboss", 1, 1, builtin_emboss}},
      {"edge_detect", {"edge_detect", 1, 1, builtin_edge_detect}},
  };
}
