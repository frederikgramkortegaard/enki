#include "../../../interpreter/eval.hpp"
#include <cstdint>
#include <vector>

// Pure C++ implementation of greyscale
extern "C" Value greyscale_cpu(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("greyscale_cpu(image): expects image");
    }
    
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val) {
        throw std::runtime_error("greyscale_cpu(image): expects image");
    }
    
    auto out = std::make_shared<ImageValue>(*img_val);
    
    // Pure C++ greyscale implementation
    for (size_t i = 0; i + 2 < out->data.size(); i += 3) {
        uint8_t r = out->data[i], g = out->data[i + 1], b = out->data[i + 2];
        uint8_t gray = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
        out->data[i] = out->data[i + 1] = out->data[i + 2] = gray;
    }
    
    return out;
}