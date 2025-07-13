#include <cuda_runtime.h>
#include "../../../interpreter/eval.hpp"

// CUDA kernel - pure GPU computation
__global__ void greyscale_kernel(unsigned char* input, unsigned char* output, int width, int height) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int pixel_idx = idx * 3;
    
    if (idx < width * height) {
        unsigned char r = input[pixel_idx];
        unsigned char g = input[pixel_idx + 1];
        unsigned char b = input[pixel_idx + 2];
        
        unsigned char gray = static_cast<unsigned char>(0.299f * r + 0.587f * g + 0.114f * b);
        
        output[pixel_idx] = gray;
        output[pixel_idx + 1] = gray;
        output[pixel_idx + 2] = gray;
    }
}

// C++ wrapper that handles CUDA setup and calls the kernel
extern "C" Value greyscale_cuda(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("greyscale_cuda(image): expects image");
    }
    
    auto img_val = std::dynamic_pointer_cast<ImageValue>(args[0]);
    if (!img_val) {
        throw std::runtime_error("greyscale_cuda(image): expects image");
    }
    
    int width = img_val->width;
    int height = img_val->height;
    int data_size = width * height * 3;
    
    // CUDA memory management and kernel launch
    unsigned char *d_input, *d_output;
    cudaMalloc(&d_input, data_size);
    cudaMalloc(&d_output, data_size);
    
    cudaMemcpy(d_input, img_val->data.data(), data_size, cudaMemcpyHostToDevice);
    
    int block_size = 256;
    int grid_size = (width * height + block_size - 1) / block_size;
    greyscale_kernel<<<grid_size, block_size>>>(d_input, d_output, width, height);
    cudaDeviceSynchronize();
    
    std::vector<uint8_t> output_data(data_size);
    cudaMemcpy(output_data.data(), d_output, data_size, cudaMemcpyDeviceToHost);
    
    cudaFree(d_input);
    cudaFree(d_output);
    
    return std::make_shared<ImageValue>(width, height, std::move(output_data), img_val->format);
}