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
#include <cmath>

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
Value builtin_invert(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("invert(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("invert(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    for (size_t i = 0; i + 2 < out->data.size(); i += 3) {
        out->data[i]     = 255 - out->data[i];     // R
        out->data[i + 1] = 255 - out->data[i + 1]; // G
        out->data[i + 2] = 255 - out->data[i + 2]; // B
    }
    std::cout << "Inverted image colors\n";
    return out;
}

Value builtin_threshold(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("threshold(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("threshold(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    uint8_t threshold = 128; // fixed threshold for now
    for (size_t i = 0; i + 2 < out->data.size(); i += 3) {
        uint8_t r = out->data[i], g = out->data[i+1], b = out->data[i+2];
        uint8_t gray = static_cast<uint8_t>(0.299*r + 0.587*g + 0.114*b);
        uint8_t val = (gray >= threshold) ? 255 : 0;
        out->data[i] = out->data[i+1] = out->data[i+2] = val;
    }
    std::cout << "Applied threshold to image\n";
    return out;
}

Value builtin_brightness(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("brightness(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("brightness(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    int delta = 40; // brighten by +40
    for (size_t i = 0; i < out->data.size(); ++i) {
        int v = static_cast<int>(out->data[i]) + delta;
        out->data[i] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
    std::cout << "Adjusted image brightness\n";
    return out;
}

Value builtin_contrast(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("contrast(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("contrast(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    float factor = 1.2f; // increase contrast
    for (size_t i = 0; i < out->data.size(); ++i) {
        int v = static_cast<int>((out->data[i] - 128) * factor + 128);
        out->data[i] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
    std::cout << "Adjusted image contrast\n";
    return out;
}

Value builtin_saturate(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("saturate(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("saturate(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    float factor = 1.3f; // increase saturation
    for (size_t i = 0; i + 2 < out->data.size(); i += 3) {
        float r = out->data[i], g = out->data[i+1], b = out->data[i+2];
        float gray = 0.299f*r + 0.587f*g + 0.114f*b;
        out->data[i]     = static_cast<uint8_t>(std::clamp(gray + factor*(r-gray), 0.0f, 255.0f));
        out->data[i + 1] = static_cast<uint8_t>(std::clamp(gray + factor*(g-gray), 0.0f, 255.0f));
        out->data[i + 2] = static_cast<uint8_t>(std::clamp(gray + factor*(b-gray), 0.0f, 255.0f));
    }
    std::cout << "Adjusted image saturation\n";
    return out;
}

Value builtin_exposure(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("exposure(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("exposure(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    float factor = 1.1f; // simple exposure adjustment
    for (size_t i = 0; i < out->data.size(); ++i) {
        int v = static_cast<int>(out->data[i] * factor);
        out->data[i] = static_cast<uint8_t>(std::clamp(v, 0, 255));
    }
    std::cout << "Adjusted image exposure\n";
    return out;
}

Value builtin_resize(const std::vector<Value>& args) {
    if (args.size() != 2 && args.size() != 1)
        throw std::runtime_error("resize(image[, ...]): expects at least image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("resize(image): expects image");
    // For now, just return a copy (no actual resize)
    std::cout << "Stub: resize returns copy of image\n";
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
    std::cout << "Rotated image by " << angle << " degrees\n";
    return std::make_shared<ImageValue>(new_w, new_h, std::move(out_data), img_val->format);
}
Value builtin_flip(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("flip(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("flip(image): expects image");
    // For now, just return a copy (no actual flip)
    std::cout << "Stub: flip returns copy of image\n";
    return std::make_shared<ImageValue>(*img_val);
}
Value builtin_crop(const std::vector<Value>& args) {
    if (args.size() < 1)
        throw std::runtime_error("crop(image, ...): expects at least image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("crop(image): expects image");
    // For now, just return a copy (no actual crop)
    std::cout << "Stub: crop returns copy of image\n";
    return std::make_shared<ImageValue>(*img_val);
}
Value builtin_blur(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("blur(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("blur(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    // Simple box blur (3x3 kernel)
    int w = out->width, h = out->height;
    std::vector<uint8_t> orig = out->data;
    for (int y = 1; y < h-1; ++y) {
        for (int x = 1; x < w-1; ++x) {
            for (int c = 0; c < 3; ++c) {
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        sum += orig[3*((y+dy)*w + (x+dx)) + c];
                out->data[3*(y*w + x) + c] = static_cast<uint8_t>(sum/9);
            }
        }
    }
    std::cout << "Applied blur to image\n";
    return out;
}

Value builtin_sharpen(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("sharpen(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("sharpen(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    // Simple sharpen kernel
    int w = out->width, h = out->height;
    std::vector<uint8_t> orig = out->data;
    int kernel[3][3] = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};
    for (int y = 1; y < h-1; ++y) {
        for (int x = 1; x < w-1; ++x) {
            for (int c = 0; c < 3; ++c) {
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        sum += kernel[dy+1][dx+1] * orig[3*((y+dy)*w + (x+dx)) + c];
                sum = std::clamp(sum, 0, 255);
                out->data[3*(y*w + x) + c] = static_cast<uint8_t>(sum);
            }
        }
    }
    std::cout << "Applied sharpen to image\n";
    return out;
}
Value builtin_emboss(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("emboss(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("emboss(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    int w = out->width, h = out->height;
    std::vector<uint8_t> orig = out->data;
    int kernel[3][3] = {{-2, -1, 0}, {-1, 1, 1}, {0, 1, 2}};
    for (int y = 1; y < h-1; ++y) {
        for (int x = 1; x < w-1; ++x) {
            for (int c = 0; c < 3; ++c) {
                int sum = 128;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        sum += kernel[dy+1][dx+1] * orig[3*((y+dy)*w + (x+dx)) + c];
                sum = std::clamp(sum, 0, 255);
                out->data[3*(y*w + x) + c] = static_cast<uint8_t>(sum);
            }
        }
    }
    std::cout << "Applied emboss to image\n";
    return out;
}

Value builtin_edge_detect(const std::vector<Value>& args) {
    if (args.size() != 1)
        throw std::runtime_error("edge_detect(image): expects image");
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val)
        throw std::runtime_error("edge_detect(image): expects image");
    auto out = std::make_shared<ImageValue>(*img_val);
    int w = out->width, h = out->height;
    std::vector<uint8_t> orig = out->data;
    int kernel[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
    for (int y = 1; y < h-1; ++y) {
        for (int x = 1; x < w-1; ++x) {
            for (int c = 0; c < 3; ++c) {
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        sum += kernel[dy+1][dx+1] * orig[3*((y+dy)*w + (x+dx)) + c];
                sum = std::clamp(sum, 0, 255);
                out->data[3*(y*w + x) + c] = static_cast<uint8_t>(sum);
            }
        }
    }
    std::cout << "Applied edge detection to image\n";
    return out;
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
      {"invert", {"invert", 1, 1, builtin_invert}},
      {"threshold", {"threshold", 1, 1, builtin_threshold}},
      {"brightness", {"brightness", 1, 1, builtin_brightness}},
      {"contrast", {"contrast", 1, 1, builtin_contrast}},
      {"saturate", {"saturate", 1, 1, builtin_saturate}},
      {"exposure", {"exposure", 1, 1, builtin_exposure}},
      {"resize", {"resize", 2, 2, builtin_resize}},
      {"rotate", {"rotate", 2, 2, builtin_rotate}},
      {"flip", {"flip", 1, 1, builtin_flip}},
      {"crop", {"crop", 5, 5, builtin_crop}},
      {"blur", {"blur", 1, 1, builtin_blur}},
      {"sharpen", {"sharpen", 1, 1, builtin_sharpen}},
      {"emboss", {"emboss", 1, 1, builtin_emboss}},
      {"edge_detect", {"edge_detect", 1, 1, builtin_edge_detect}},
  };
}
